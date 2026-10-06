// Interfaz web. Toda la logica del lexer vive en C++ (lexer.wasm);
// aqui solo se mueve texto entre la pagina y el modulo.
(function () {
  'use strict';

  var MAX_BYTES = 1024 * 1024; // 1 MB: suficiente para esta entrega
  var $ = function (id) { return document.getElementById(id); };
  var source = $('source'), output = $('output'), status = $('status');
  var runBtn = $('run'), fileInput = $('file'), drop = $('drop');
  var lexer = null;

  function selectedMode() {
    return document.querySelector('input[name="mode"]:checked').value;
  }

  function normalize(text) {
    // Igual que el CLI: quita BOM y convierte \r\n en \n.
    if (text.charCodeAt(0) === 0xFEFF) text = text.slice(1);
    return text.replace(/\r\n/g, '\n');
  }

  function run() {
    if (!lexer) return;
    var text = normalize(source.value);
    output.textContent = selectedMode() === 'rubric'
      ? lexer.analyzeRubric(text)
      : lexer.analyzeTable(text);
  }

  function loadFile(file) {
    if (!file) return;
    if (file.size > MAX_BYTES) {
      status.textContent = 'Archivo demasiado grande (máx. 1 MB).';
      return;
    }
    // File.text() lee el archivo local en el navegador (UTF-8).
    file.text().then(function (text) {
      source.value = normalize(text);
      status.textContent = 'Archivo: ' + file.name;
      run();
    }).catch(function () {
      status.textContent = 'No se pudo leer el archivo.';
    });
  }

  fileInput.addEventListener('change', function () { loadFile(fileInput.files[0]); });
  runBtn.addEventListener('click', run);
  $('clear').addEventListener('click', function () {
    source.value = ''; output.textContent = ''; fileInput.value = '';
    status.textContent = lexer ? 'Listo.' : status.textContent;
  });
  Array.prototype.forEach.call(document.querySelectorAll('input[name="mode"]'),
    function (r) { r.addEventListener('change', run); });

  ['dragenter', 'dragover'].forEach(function (ev) {
    drop.addEventListener(ev, function (e) { e.preventDefault(); drop.classList.add('over'); });
  });
  ['dragleave', 'drop'].forEach(function (ev) {
    drop.addEventListener(ev, function (e) { e.preventDefault(); drop.classList.remove('over'); });
  });
  drop.addEventListener('drop', function (e) {
    loadFile(e.dataTransfer.files[0]);
  });

  createLexerModule().then(function (module) {
    lexer = module;
    runBtn.disabled = false;
    status.textContent = 'Listo.';
    run();
  }).catch(function (err) {
    status.textContent = 'Error cargando WebAssembly: ' + err;
  });
})();
