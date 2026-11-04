// Esecuzione del solver compilato in WebAssembly (WASI) dentro il browser, per l'analisi di uno slot.
// Implementa il minimo di WASI che serve al solver: argomenti, orologio, e un filesystem in memoria
// (i file di ingresso arrivano come stringhe, stdout torna come stringa). Ogni esecuzione ha memoria
// fresca: come lanciare il processo, nessuno stato resta tra una chiamata e l'altra.
// Usato da orario.js (finestra della coda); gira anche in Node per i test.
(function () {
  'use strict';
  const ERRNO = {OK: 0, BADF: 8, NOENT: 44, NOSYS: 52, INVAL: 28};

  // Esegue il modulo (gia' compilato con WebAssembly.compile) con gli argomenti e i file dati
  // (nome -> stringa o Uint8Array). Ritorna {stdout, codice, files} (files: anche quelli scritti).
  async function eseguiWasi(modulo, argomenti, files) {
    const enc = new TextEncoder(), dec = new TextDecoder();
    const argv = argomenti.map(a => enc.encode(a + '\0'));
    const fs = {};  // nome -> Uint8Array
    for (const n in files) fs[n] = typeof files[n] === 'string' ? enc.encode(files[n]) : files[n];
    // descrittori aperti: 0/1/2 standard, 3 la directory radice (preopen), poi i file
    const aperti = {};  // fd -> {nome, pos}
    let prossimoFd = 4;
    let stdout = '', stderr = '';
    let memoria = null;
    const m = () => new DataView(memoria.buffer);
    const bytes = (p, n) => new Uint8Array(memoria.buffer, p, n);
    const esci = {};  // lanciato da proc_exit

    let codice = 0;
    const wasi = {
      args_sizes_get(pArgc, pLen) {
        m().setUint32(pArgc, argv.length, true);
        m().setUint32(pLen, argv.reduce((s, a) => s + a.length, 0), true);
        return ERRNO.OK;
      },
      args_get(pArgv, pBuf) {
        for (const a of argv) {
          m().setUint32(pArgv, pBuf, true);
          bytes(pBuf, a.length).set(a);
          pArgv += 4; pBuf += a.length;
        }
        return ERRNO.OK;
      },
      environ_sizes_get(pN, pLen) { m().setUint32(pN, 0, true); m().setUint32(pLen, 0, true); return ERRNO.OK; },
      environ_get() { return ERRNO.OK; },
      clock_time_get(id, _prec, pTime) { m().setBigUint64(pTime, BigInt(Math.round(Date.now() * 1e6)), true); return ERRNO.OK; },
      clock_res_get(id, pRes) { m().setBigUint64(pRes, 1000000n, true); return ERRNO.OK; },
      random_get(p, n) { const b = bytes(p, n); for (let i = 0; i < n; i++) b[i] = Math.floor(Math.random() * 256); return ERRNO.OK; },
      proc_exit(c) { codice = c; throw esci; },
      sched_yield() { return ERRNO.OK; },
      poll_oneoff() { return ERRNO.NOSYS; },

      fd_prestat_get(fd, pBuf) {
        if (fd !== 3) return ERRNO.BADF;
        m().setUint8(pBuf, 0);           // 0 = directory
        m().setUint32(pBuf + 4, 1, true); // lunghezza del nome "/"
        return ERRNO.OK;
      },
      fd_prestat_dir_name(fd, pPath, len) {
        if (fd !== 3) return ERRNO.BADF;
        bytes(pPath, 1).set(enc.encode('/'));
        return ERRNO.OK;
      },
      path_open(dirfd, _dirflags, pPath, lenPath, oflags, _rb, _ri, _fdflags, pFd) {
        let nome = dec.decode(bytes(pPath, lenPath));
        if (nome.startsWith('/')) nome = nome.slice(1);
        const crea = (oflags & 1) !== 0, tronca = (oflags & 8) !== 0;
        if (!(nome in fs)) {
          if (!crea) return ERRNO.NOENT;
          fs[nome] = new Uint8Array(0);
        } else if (tronca) fs[nome] = new Uint8Array(0);
        const fd = prossimoFd++;
        aperti[fd] = {nome, pos: 0};
        m().setUint32(pFd, fd, true);
        return ERRNO.OK;
      },
      fd_read(fd, pIovs, nIovs, pLetti) {
        const f = aperti[fd];
        if (!f) return ERRNO.BADF;
        let letti = 0;
        for (let i = 0; i < nIovs; i++) {
          const p = m().getUint32(pIovs + i * 8, true), n = m().getUint32(pIovs + i * 8 + 4, true);
          const resto = fs[f.nome].length - f.pos;
          const q = Math.min(n, resto);
          if (q > 0) { bytes(p, q).set(fs[f.nome].subarray(f.pos, f.pos + q)); f.pos += q; letti += q; }
          if (q < n) break;
        }
        m().setUint32(pLetti, letti, true);
        return ERRNO.OK;
      },
      fd_write(fd, pIovs, nIovs, pScritti) {
        let scritti = 0;
        for (let i = 0; i < nIovs; i++) {
          const p = m().getUint32(pIovs + i * 8, true), n = m().getUint32(pIovs + i * 8 + 4, true);
          const pezzo = bytes(p, n);
          if (fd === 1) stdout += dec.decode(pezzo);
          else if (fd === 2) stderr += dec.decode(pezzo);
          else {
            const f = aperti[fd];
            if (!f) return ERRNO.BADF;
            const vecchio = fs[f.nome];
            const nuovo = new Uint8Array(Math.max(vecchio.length, f.pos + n));
            nuovo.set(vecchio); nuovo.set(pezzo, f.pos);
            fs[f.nome] = nuovo; f.pos += n;
          }
          scritti += n;
        }
        m().setUint32(pScritti, scritti, true);
        return ERRNO.OK;
      },
      fd_seek(fd, offset, whence, pNuovo) {
        const f = aperti[fd];
        if (!f) { if (fd <= 2) { m().setBigUint64(pNuovo, 0n, true); return ERRNO.OK; } return ERRNO.BADF; }
        const base = whence === 0 ? 0 : whence === 1 ? f.pos : fs[f.nome].length;
        f.pos = base + Number(offset);
        m().setBigUint64(pNuovo, BigInt(f.pos), true);
        return ERRNO.OK;
      },
      fd_close(fd) { delete aperti[fd]; return ERRNO.OK; },
      fd_fdstat_get(fd, pBuf) {
        // filetype: 2 = character device (stdout/err), 4 = file normale
        m().setUint8(pBuf, fd <= 2 ? 2 : 4);
        m().setUint16(pBuf + 2, 0, true);
        m().setBigUint64(pBuf + 8, 0xffffffffffffffffn, true);
        m().setBigUint64(pBuf + 16, 0xffffffffffffffffn, true);
        return ERRNO.OK;
      },
      fd_fdstat_set_flags() { return ERRNO.OK; },
      fd_filestat_get(fd, pBuf) {
        const f = aperti[fd];
        if (!f) return ERRNO.BADF;
        bytes(pBuf, 64).fill(0);
        m().setUint8(pBuf + 16, 4);  // filetype: file normale
        m().setBigUint64(pBuf + 32, BigInt(fs[f.nome].length), true);
        return ERRNO.OK;
      },
      path_filestat_get(dirfd, _flags, pPath, lenPath, pBuf) {
        let nome = dec.decode(bytes(pPath, lenPath));
        if (nome.startsWith('/')) nome = nome.slice(1);
        if (!(nome in fs)) return ERRNO.NOENT;
        bytes(pBuf, 64).fill(0);
        m().setUint8(pBuf + 16, 4);
        m().setBigUint64(pBuf + 32, BigInt(fs[nome].length), true);
        return ERRNO.OK;
      },
      path_unlink_file() { return ERRNO.OK; },
    };

    const istanza = await WebAssembly.instantiate(modulo, {wasi_snapshot_preview1: wasi});
    memoria = istanza.exports.memory;
    try {
      istanza.exports._start();
    } catch (e) {
      if (e !== esci) throw e;
    }
    return {stdout, stderr, codice, files: fs};
  }

  (typeof window !== 'undefined' ? window : globalThis).eseguiWasi = eseguiWasi;
})();
