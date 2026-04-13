const fs = require('fs');
const path = require('path');

const target = process.argv[2] || 'build-web';
const suffix = target.includes('DEV') ? '_DEV' : '';

const buildDir = path.join(__dirname, target);
const outputFile = path.join(__dirname, `RSDKv3${suffix}_SingleFile.html`);

if (!fs.existsSync(buildDir)) {
    console.error(`Error: Directory ${target} does not exist.`);
    process.exit(1);
}

console.log(`Starting improved single-file compilation for ${target}...`);

// 1. Read the base HTML
let html = fs.readFileSync(path.join(buildDir, 'RSDKv3.html'), 'utf8');

// 2. Read the JS
let js = fs.readFileSync(path.join(buildDir, 'RSDKv3.js'), 'utf8');

// 3. Encode WASM
console.log('Encoding WASM...');
const wasmBuffer = fs.readFileSync(path.join(buildDir, 'RSDKv3.wasm'));
const wasmBase64 = wasmBuffer.toString('base64');

// Start writing the output file
const out = fs.createWriteStream(outputFile);

// Remove the original script tag from HTML (it might be src=RSDKv3.js or src="RSDKv3.js")
html = html.replace(/<script[^>]*src="?RSDKv3\.js"? [^>]*><\/script>/i, '');
html = html.replace(/<script[^>]*src="?RSDKv3\.js"?[^>]*><\/script>/i, '');

// Split HTML to inject our data
let htmlParts = html.split('</head>');
if (htmlParts.length < 2) {
    htmlParts = html.split('<body>');
}

out.write(htmlParts[0]);
out.write('<head>');

// Inject WASM, reassembly logic, and Module overrides
out.write(`
<script>
  window.Module = window.Module || {};
  (function() {
    console.log('Initializing reassembly...');
    const wasmBase64 = "${wasmBase64}";
    
    function base64ToArrayBuffer(base64) {
      var binary_string = window.atob(base64);
      var len = binary_string.length;
      var bytes = new Uint8Array(len);
      for (var i = 0; i < len; i++) bytes[i] = binary_string.charCodeAt(i);
      return bytes.buffer;
    }

    const wasmBlob = new Blob([base64ToArrayBuffer(wasmBase64)], { type: 'application/wasm' });
    const wasmUrl = URL.createObjectURL(wasmBlob);
    
    window._rsdkDataChunks = [];
    window._rsdkRegisterChunk = function(idx, base64) {
        window._rsdkDataChunks[idx] = base64;
    };

    window._rsdkFinalize = function() {
        console.log('Reassembling data chunks...');
        const totalSize = ${fs.statSync(path.join(buildDir, 'RSDKv3.data')).size};
        const finalBuffer = new Uint8Array(totalSize);
        let offset = 0;
        
        for (let i = 0; i < window._rsdkDataChunks.length; i++) {
            const binary_string = window.atob(window._rsdkDataChunks[i]);
            for (let j = 0; j < binary_string.length; j++) {
                finalBuffer[offset++] = binary_string.charCodeAt(j);
            }
            window._rsdkDataChunks[i] = null;
        }
        
        const dataBlob = new Blob([finalBuffer], { type: 'application/octet-stream' });
        const dataUrl = URL.createObjectURL(dataBlob);

        window.Module.locateFile = function(path, prefix) {
          if (path.endsWith('.data')) return dataUrl;
          if (path.endsWith('.wasm')) return wasmUrl;
          return prefix + path;
        };
        console.log('Data reassembled. Launching engine...');
    };
  })();
</script>
`);

out.write(htmlParts[1]);

// Streaming DATA chunks
console.log('Streaming DATA chunks...');
const dataBuffer = fs.readFileSync(path.join(buildDir, 'RSDKv3.data'));
const CHUNK_SIZE = 10 * 1024 * 1024;
const totalChunks = Math.ceil(dataBuffer.length / CHUNK_SIZE);

for (let i = 0; i < totalChunks; i++) {
    const chunk = dataBuffer.slice(i * CHUNK_SIZE, Math.min((i + 1) * CHUNK_SIZE, dataBuffer.length));
    const base64 = chunk.toString('base64');
    out.write(`<script>_rsdkRegisterChunk(${i}, "${base64}");</script>\n`);
}

// Finalize and inject main JS
out.write(`<script>_rsdkFinalize();</script>\n`);

// Escape script tags in the JS itself to prevent premature closing
js = js.replace(/<\/script>/g, '<\\/script>');
out.write(`<script type="text/javascript">${js}</script>`);

out.end(() => {
    console.log('Done! Single-file HTML created at: ' + outputFile);
});
