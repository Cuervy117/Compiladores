// Interfaz web. Toda la logica del compilador vive en C++ (compiler.wasm);
// aqui solo se dibuja lo que regresa en JSON.
//
// Cada fase avisa su resultado con un evento sobre document:
//   analysis:done, lexer:done, parser:ok, parser:error,
//   sdt:ok, sdt:error, sdt:warning, sdt:skipped
// y los listeners de abajo actualizan los recuadros, los mensajes y los avisos.
(function () {
  'use strict';

  var MAX_BYTES = 1024 * 1024; // 1 MB
  var BIG_TREE = 1500;         // aristas; arriba de esto el arbol completo tarda
  var $ = function (id) { return document.getElementById(id); };

  var compiler = null;   // modulo de WebAssembly
  var last = null;       // ultimo resultado (JSON ya convertido)
  var zoom = 1;
  var renderCount = 0;
  var traceDrawn = false;

  // ---------------------------------------------------------------- utilidades

  function emit(name, detail) {
    document.dispatchEvent(new CustomEvent(name, { detail: detail }));
  }

  function on(name, fn) {
    document.addEventListener(name, function (e) { fn(e.detail); });
  }

  function el(tag, className, text) {
    var node = document.createElement(tag);
    if (className) node.className = className;
    if (text !== undefined) node.textContent = text;
    return node;
  }

  function clear(node) {
    while (node.firstChild) node.removeChild(node.firstChild);
  }

  function row(cells, tag) {
    var tr = document.createElement('tr');
    cells.forEach(function (c) { tr.appendChild(el(tag || 'td', '', c)); });
    return tr;
  }

  function normalize(text) {
    // Igual que el CLI: quita BOM y convierte \r\n en \n
    if (text.charCodeAt(0) === 0xFEFF) text = text.slice(1);
    return text.replace(/\r\n/g, '\n');
  }

  function plural(n, word) {
    return n + ' ' + word + (n === 1 ? '' : 's');
  }

  function toast(kind, text) {
    var box = el('div', 'toast ' + kind, text);
    $('toasts').appendChild(box);
    setTimeout(function () { box.classList.add('hide'); }, 4000);
    setTimeout(function () { box.remove(); }, 4500);
  }

  function setStage(id, kind, text) {
    var stage = $(id);
    stage.className = 'stage ' + kind;
    stage.querySelector('span').textContent = text;
  }

  function download(blob, name) {
    var a = document.createElement('a');
    a.href = URL.createObjectURL(blob);
    a.download = name;
    document.body.appendChild(a);
    a.click();
    setTimeout(function () { URL.revokeObjectURL(a.href); a.remove(); }, 1000);
  }

  function copyText(text) {
    if (navigator.clipboard && window.isSecureContext) {
      return navigator.clipboard.writeText(text);
    }
    // file:// y http sin https no tienen clipboard API
    var area = el('textarea');
    area.value = text;
    document.body.appendChild(area);
    area.select();
    document.execCommand('copy');
    area.remove();
    return Promise.resolve();
  }

  // ---------------------------------------------------------------- analisis

  function run() {
    if (!compiler) return;
    var result;
    try {
      result = JSON.parse(compiler.analyze(normalize($('source').value)));
    } catch (err) {
      toast('bad', 'Internal error: ' + err);
      return;
    }
    last = result;
    clear($('toasts')); // los avisos viejos ya no aplican
    emit('analysis:done', result);
  }

  // Del resultado completo salen los eventos de cada fase
  on('analysis:done', function (r) {
    emit('lexer:done', r);
    emit(r.parse.ok ? 'parser:ok' : 'parser:error', r);
    if (!r.sdt.evaluated) emit('sdt:skipped', r);
    else emit(r.sdt.ok ? 'sdt:ok' : 'sdt:error', r);
    if (r.sdt.warnings.length) emit('sdt:warning', r);
  });

  on('analysis:done', function (r) {
    ['result', 'lexical', 'syntax', 'semantic'].forEach(function (id) { $(id).hidden = false; });
    $('summary').textContent = r.summary.join('\n');
    drawMessages(r);
    drawTokens(r);
    drawSymbols(r);
    traceDrawn = false;
    if ($('trace-details').open) drawTrace();
    drawTree();
  });

  on('lexer:done', function (r) {
    var unknown = r.counts.unknown;
    setStage('stage-lexer', unknown ? 'warn' : 'ok',
      plural(r.total, 'token') + (unknown ? ' (' + unknown + ' unknown)' : ''));
  });

  on('parser:ok', function () {
    setStage('stage-parser', 'ok', 'Parsing Success!');
  });

  on('parser:error', function (r) {
    setStage('stage-parser', 'bad', 'Parsing error in line ' + r.parse.line);
    toast('bad', 'Parsing error in line ' + r.parse.line + '. The SDT was not evaluated.');
  });

  on('sdt:skipped', function () {
    setStage('stage-sdt', 'skip', 'Not evaluated');
  });

  on('sdt:ok', function (r) {
    setStage('stage-sdt', r.sdt.warnings.length ? 'warn' : 'ok', 'SDT Verified!');
    toast('ok', 'Parsing Success! and SDT Verified!');
  });

  on('sdt:error', function (r) {
    setStage('stage-sdt', 'bad', 'SDT error: ' + plural(r.sdt.errors.length, 'problem'));
    toast('bad', 'The code is well formed but breaks ' + plural(r.sdt.errors.length, 'semantic rule') + '.');
  });

  on('sdt:warning', function (r) {
    toast('warn', plural(r.sdt.warnings.length, 'warning') + ': the code compiles, but check them.');
  });

  // ---------------------------------------------------------------- resultados

  function drawMessages(r) {
    var list = $('messages');
    var lines = normalize($('source').value).split('\n');
    clear(list);

    function add(kind, label, line, message) {
      var li = el('li', kind);
      li.appendChild(el('span', 'kind', label));
      li.appendChild(document.createTextNode('line ' + line + ': ' + message));
      var code = lines[line - 1];
      if (code !== undefined && code.trim() !== '') {
        li.appendChild(el('pre', '', String(line).padStart(4) + ' | ' + code));
      }
      list.appendChild(li);
    }

    if (!r.parse.ok) {
      var lexical = r.parse.message.indexOf('lexical error') === 0;
      add('error', lexical ? 'Lexical error' : 'Syntax error', r.parse.line, r.parse.message);
    }
    r.sdt.errors.forEach(function (d) { add('error', 'Semantic error', d.line, d.message); });
    r.sdt.warnings.forEach(function (d) { add('warning', 'Warning', d.line, d.message); });
  }

  function drawTokens(r) {
    var body = $('token-table').querySelector('tbody');
    clear(body);
    r.tokens.forEach(function (t) {
      body.appendChild(row([t.n, t.lexeme, t.category, t.line, t.terminal]));
    });
    $('token-total').textContent = r.total;

    var counts = $('count-table').querySelector('tbody');
    clear(counts);
    Object.keys(r.counts).forEach(function (k) {
      if (k === 'unknown' && r.counts[k] === 0) return;
      counts.appendChild(row([k, r.counts[k]]));
    });
  }

  function drawSymbols(r) {
    var body = $('symbol-table').querySelector('tbody');
    clear(body);
    if (!r.sdt.evaluated) {
      var tr = row(['The SDT was not evaluated, so nothing was declared.']);
      tr.firstChild.colSpan = 6;
      body.appendChild(tr);
      return;
    }
    r.symbols.forEach(function (s) {
      var tr = row([s.name, s.kind, s.type, s.params || '-', s.line, s.scope]);
      tr.lastChild.className = 'depth-' + Math.min(s.depth, 3);
      body.appendChild(tr);
    });
    if (!r.symbols.length) body.appendChild(row(['(empty)', '', '', '', '', '']));
  }

  function drawTrace() {
    if (!last || traceDrawn) return;
    var body = $('trace-table').querySelector('tbody');
    clear(body);
    last.trace.forEach(function (step, i) {
      var tr = row([i + 1, step[0], step[1], step[2]]);
      if (step[2] === 'ERROR') tr.style.background = '#fdecec';
      body.appendChild(tr);
    });
    $('trace-note').textContent = last.trace.length + ' steps' +
      (last.traceTruncated ? ' (only the first ones are shown).' : '.');
    traceDrawn = true;
  }

  // ---------------------------------------------------------------- arbol

  function currentDiagram() {
    if (!last) return '';
    return $('compact').checked ? last.tree.compact : last.tree.full;
  }

  function treeSize(svg) {
    var vb = svg.viewBox.baseVal;
    return { w: vb && vb.width ? vb.width : 800, h: vb && vb.height ? vb.height : 600 };
  }

  function applyZoom() {
    var svg = $('tree-box').querySelector('svg');
    if (!svg) return;
    var size = treeSize(svg);
    svg.style.maxWidth = 'none';
    svg.setAttribute('width', Math.round(size.w * zoom));
    svg.setAttribute('height', Math.round(size.h * zoom));
  }

  function fitTree() {
    var svg = $('tree-box').querySelector('svg');
    if (!svg) return;
    zoom = Math.min(1, ($('tree-box').clientWidth - 24) / treeSize(svg).w);
    applyZoom();
  }

  function drawTree() {
    var box = $('tree-box');
    var code = currentDiagram();
    var edges = (code.match(/-->/g) || []).length;
    var id = 'parse-tree-' + (++renderCount);

    box.textContent = edges > BIG_TREE
      ? 'Drawing a big tree (' + edges + ' edges), this can take a few seconds...'
      : 'Drawing the tree...';

    // Se deja que el navegador pinte el mensaje antes de que Mermaid trabaje
    setTimeout(function () {
      mermaid.render(id, code).then(function (out) {
        if (id !== 'parse-tree-' + renderCount) return; // ya hay otro mas nuevo
        box.innerHTML = out.svg;
        fitTree();
      }).catch(function (err) {
        box.textContent = 'Mermaid could not draw the tree: ' + (err && err.message ? err.message : err);
      });
    }, 30);
  }

  function svgForExport() {
    var svg = $('tree-box').querySelector('svg');
    if (!svg) return null;
    var size = treeSize(svg);
    var copy = svg.cloneNode(true);
    copy.setAttribute('xmlns', 'http://www.w3.org/2000/svg');
    copy.setAttribute('width', size.w);
    copy.setAttribute('height', size.h);
    copy.style.maxWidth = '';
    return { text: new XMLSerializer().serializeToString(copy), w: size.w, h: size.h };
  }

  function downloadSvg() {
    var svg = svgForExport();
    if (!svg) return toast('warn', 'Compile something first.');
    download(new Blob([svg.text], { type: 'image/svg+xml' }), 'parse-tree.svg');
  }

  function downloadPng() {
    var svg = svgForExport();
    if (!svg) return toast('warn', 'Compile something first.');

    // Los navegadores limitan el tamano del canvas; los arboles grandes se reducen
    var scale = Math.min(2, 16000 / svg.w, 16000 / svg.h, Math.sqrt(120000000 / (svg.w * svg.h)));
    var img = new Image();
    img.onload = function () {
      var canvas = document.createElement('canvas');
      canvas.width = Math.round(svg.w * scale);
      canvas.height = Math.round(svg.h * scale);
      var ctx = canvas.getContext('2d');
      ctx.fillStyle = '#ffffff';
      ctx.fillRect(0, 0, canvas.width, canvas.height);
      ctx.drawImage(img, 0, 0, canvas.width, canvas.height);
      canvas.toBlob(function (blob) {
        if (blob) download(blob, 'parse-tree.png');
        else toast('bad', 'The browser could not create the PNG. Try the SVG.');
      }, 'image/png');
    };
    img.onerror = function () { toast('bad', 'The browser could not create the PNG. Try the SVG.'); };
    img.src = 'data:image/svg+xml;charset=utf-8,' + encodeURIComponent(svg.text);
  }

  // ---------------------------------------------------------------- gramatica

  function drawGrammar(g) {
    var prodText = {};
    var prodBody = $('production-table').querySelector('tbody');
    var previous = null;
    g.productions.forEach(function (p) {
      var rhs = p.rhs.length ? p.rhs.join(' ') : 'ε';
      prodText[p.n] = p.lhs + ' → ' + rhs;
      prodBody.appendChild(row(['(' + p.n + ')', p.lhs === previous ? '' : p.lhs, p.lhs === previous ? '|' : '→', rhs]));
      previous = p.lhs;
    });

    var ff = $('first-follow-table').querySelector('tbody');
    g.nonterminals.forEach(function (a) {
      ff.appendChild(row([a, '{ ' + g.first[a].join(', ') + ' }', '{ ' + g.follow[a].join(', ') + ' }']));
    });

    var conflicts = {};
    g.conflicts.forEach(function (c) { conflicts[c.nonterminal + ' ' + c.terminal] = c; });

    var table = $('ll1-table');
    var head = document.createElement('thead');
    head.appendChild(row([''].concat(g.terminals), 'th'));
    table.appendChild(head);
    var body = document.createElement('tbody');
    g.nonterminals.forEach(function (a) {
      var tr = document.createElement('tr');
      tr.appendChild(el('th', '', a));
      g.terminals.forEach(function (t) {
        var n = g.table[a][t];
        var td = el('td', '', n === undefined ? '' : n);
        if (n !== undefined) td.title = 'M[' + a + ', ' + t + '] = (' + n + ') ' + prodText[n];
        var c = conflicts[a + ' ' + t];
        if (c) {
          td.className = 'conflict';
          td.title += '\nConflict: (' + c.kept + ') and (' + c.dropped + '), we keep (' + c.kept + ')';
        }
        tr.appendChild(td);
      });
      body.appendChild(tr);
    });
    table.appendChild(body);

    $('conflict-note').textContent = g.conflicts.map(function (c) {
      return 'The highlighted cell M[' + c.nonterminal + ', ' + c.terminal + '] is the dangling else: it had (' +
        c.kept + ') and (' + c.dropped + '); we keep (' + c.kept + ') so the else goes with the nearest if.';
    }).join(' ');
  }

  // ---------------------------------------------------------------- ejemplos

  function loadCode(code, label) {
    $('source').value = code;
    $('status').textContent = label;
    run();
    $('input').scrollIntoView({ behavior: 'smooth' });
  }

  function drawSamples() {
    var select = $('example-select');
    var list = $('sample-list');
    EXAMPLES.forEach(function (ex, i) {
      select.appendChild(el('option', '', ex.name));
      select.lastChild.value = i;

      var box = el('div', 'sample');
      var title = el('h3', '', ex.name + ' ');
      title.appendChild(el('span', 'tag ' + ex.kind, ex.expected));
      box.appendChild(title);
      box.appendChild(el('p', 'note', ex.description + ' (' + ex.file + ')'));
      box.appendChild(el('pre', '', ex.code));
      var load = el('button', 'small', 'Load');
      var copy = el('button', 'small secondary', 'Copy');
      load.addEventListener('click', function () { loadCode(ex.code, 'Example: ' + ex.name); });
      copy.addEventListener('click', function () {
        copyText(ex.code).then(function () { toast('ok', 'Copied "' + ex.name + '".'); });
      });
      box.appendChild(load);
      box.appendChild(document.createTextNode(' '));
      box.appendChild(copy);
      list.appendChild(box);
    });

    select.addEventListener('change', function () {
      if (select.value === '') return;
      var ex = EXAMPLES[select.value];
      loadCode(ex.code, 'Example: ' + ex.name);
    });
  }

  // ---------------------------------------------------------------- archivos

  function loadFile(file) {
    if (!file) return;
    if (file.size > MAX_BYTES) {
      toast('bad', 'File too big (max. 1 MB).');
      return;
    }
    file.text().then(function (text) {
      $('source').value = normalize(text);
      $('status').textContent = 'File: ' + file.name;
      run();
    }).catch(function () {
      toast('bad', 'Could not read the file.');
    });
  }

  // ---------------------------------------------------------------- arranque

  $('file').addEventListener('change', function () { loadFile($('file').files[0]); });
  $('run').addEventListener('click', run);
  $('clear').addEventListener('click', function () {
    $('source').value = '';
    $('file').value = '';
    $('example-select').value = '';
    ['result', 'lexical', 'syntax', 'semantic'].forEach(function (id) { $(id).hidden = true; });
    last = null;
  });
  $('source').addEventListener('keydown', function (e) {
    if (e.key === 'Enter' && (e.ctrlKey || e.metaKey)) { e.preventDefault(); run(); }
  });

  var drop = $('drop');
  ['dragenter', 'dragover'].forEach(function (ev) {
    drop.addEventListener(ev, function (e) { e.preventDefault(); drop.classList.add('over'); });
  });
  ['dragleave', 'drop'].forEach(function (ev) {
    drop.addEventListener(ev, function (e) { e.preventDefault(); drop.classList.remove('over'); });
  });
  drop.addEventListener('drop', function (e) { loadFile(e.dataTransfer.files[0]); });

  $('compact').addEventListener('change', drawTree);
  $('zoom-in').addEventListener('click', function () { zoom *= 1.25; applyZoom(); });
  $('zoom-out').addEventListener('click', function () { zoom /= 1.25; applyZoom(); });
  $('zoom-fit').addEventListener('click', fitTree);
  $('download-svg').addEventListener('click', downloadSvg);
  $('download-png').addEventListener('click', downloadPng);
  $('copy-mermaid').addEventListener('click', function () {
    if (!last) return toast('warn', 'Compile something first.');
    copyText(currentDiagram()).then(function () {
      toast('ok', 'Mermaid code copied. You can paste it in mermaid.live or in a README.');
    });
  });
  $('trace-details').addEventListener('toggle', function () {
    if ($('trace-details').open) drawTrace();
  });

  // htmlLabels en false: las etiquetas quedan como texto SVG y asi se puede
  // exportar a PNG (con HTML adentro el canvas queda bloqueado)
  mermaid.initialize({
    startOnLoad: false,
    securityLevel: 'strict',
    maxEdges: 5000,
    maxTextSize: 1000000,
    flowchart: { htmlLabels: false, useMaxWidth: false, nodeSpacing: 18, rankSpacing: 36 }
  });

  drawSamples();
  $('source').value = EXAMPLES[1].code;

  if (typeof createCompilerModule !== 'function') {
    $('status').textContent = 'compiler.js was not found. Build it with wasm/build_wasm.sh.';
    return;
  }

  createCompilerModule().then(function (module) {
    compiler = module;
    drawGrammar(JSON.parse(compiler.grammar()));
    $('run').disabled = false;
    $('status').textContent = 'Ready.';
    emit('compiler:ready', null);
    run();
  }).catch(function (err) {
    $('status').textContent = 'Error loading WebAssembly: ' + err;
  });
})();
