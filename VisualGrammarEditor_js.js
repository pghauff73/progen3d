Skip to main content


Search
Search

Pages
Images
Documents

Reports

Settings


Help


pamela_hauff
Editing Script page: Grammar Reader WebGL

Grammar Reader WebGL





Live
Collapse all

Title*
Html content
Codeblocks*
// Math and Grammar Sp…*
// WebGL Renderer //…*
// Scene "use strict"…*
/*Scope===============…*
/*Variable============…*
/* ===================…*
/* ================= R…*
// Grammar Lexer (keep…*
//====================…*
/* EXECUTION PIPE: pr…*
/* Pretty printers ===…*
/* = Integrated Editor…*
/* Smart Editor v0.…*
/* Grammar Fragments …*
Content
Promote
Title*
The page title as you'd like it to be seen by the public
Grammar Reader WebGL


Html content
Enter HTML content to display on the page.
<style>
  /* ===== Theme (keep variable names) ===== */
  :root{
    --bg:#0f1116;
    --card:#171b26cc;
    --glass:rgba(23,27,38,0.82);
    --text:#e6e9ef;
    --muted:#9aa4b2;
    --accent:#6ea8fe;
    --accent-2:#72e3a6;
    --radius:16px;
    --shadow:0 10px 30px rgba(0,0,0,.35);
    --green:#93f9b9;
    --blue:#8ab4ff;
    --orange:#ffb86b;

    /* derived */
    --border-soft: rgba(154,164,178,0.22);
    --border-med:  rgba(154,164,178,0.26);
    --border-strong: rgba(154,164,178,0.30);

    --smart-bg: rgba(23,27,38,0.80);
    --smart-border: rgba(154,164,178,0.25);
    --smart-shadow: 0 10px 30px rgba(0,0,0,.35);

    /* compact toolbar sizing */
    --ctl-h: 32px;          /* control height */
    --ctl-gap: 6px;         /* gap between items */
    --btn-pad-x: 10px;      /* horizontal padding for buttons */
    --btn-radius: 10px;
    --inp-radius: 8px;
    --fz-ctl: 12.5px;       /* font-size inside controls */
  }

  *{ box-sizing:border-box }
  html,body{ height:100% }
  body{
    margin:0; color:var(--text);
    background:
      radial-gradient(1200px 700px at 10% -10%, #1b2030 0%, transparent 40%),
      radial-gradient(1200px 700px at 110% 10%, #1a2a20 0%, transparent 40%),
      var(--bg);
    font-family: Inter, ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, "Helvetica Neue", Arial;
    -webkit-font-smoothing: antialiased;
    text-rendering: optimizeLegibility;
  }

  /* ===== Layout ===== */
  .wrap{
    --gap:18px;
    width:min(1400px,96vw);
    margin:28px auto;
    display:grid; gap:var(--gap);
    grid-template-columns: 1.0fr 1.2fr 1.4fr;
    grid-auto-rows: minmax(160px, auto);
  }
  @media (max-width:1200px){
    .wrap{ grid-template-columns:1fr 1fr }
    .viewer-card,.output-card{ grid-column:1/-1 }
  }
  @media (max-width:800px){ .wrap{ grid-template-columns:1fr } }

  /* ===== Cards ===== */
  .card{
    border-radius:var(--radius);
    background:linear-gradient(180deg, var(--glass), rgba(17,22,33,0.95));
    border:1px solid var(--border-soft);
    box-shadow:var(--shadow);
    overflow:hidden;
    -webkit-backdrop-filter: blur(6px) saturate(120%);
    backdrop-filter: blur(6px) saturate(120%);
  }
  .card header{
    padding:8px 12px;                  /* tighter header */
    display:flex; align-items:center; justify-content:space-between;
    border-bottom:1px solid var(--border-soft);
    gap:10px;
  }
  .card h2{
    margin:0; font-size:12px; text-transform:uppercase; letter-spacing:.14em; color:var(--muted);
  }
  .card .body{ padding:14px; }

  /* Section spans (keep names) */
  .input-card{ grid-column:1 / span 2; }
  .viewer-card{ grid-column:3 / span 1; }
  .output-card{ grid-column:1 / -1; }

  /* ===== Compact toolbar & inputs ===== */
  .toolbar{
    display:flex; align-items:center; flex-wrap:wrap;
    gap:var(--ctl-gap);
  }
  #status{
    font-size:11.5px; color:var(--muted);
    margin-right:4px;
  }

  .text-input, .select-input{
    height:var(--ctl-h);
    min-width:200px;
    padding:0 10px;
    border-radius:var(--inp-radius);
    color:var(--text);
    background:rgba(255,255,255,.04);
    border:1px solid var(--border-med);
    font-size:var(--fz-ctl);
    font-weight:500;
    line-height:calc(var(--ctl-h) - 2px);
  }
  .text-input::placeholder{ color:var(--muted); }
  .text-input:focus, .select-input:focus{
    outline:none;
    box-shadow:0 0 0 2px rgba(110,168,254,.25) inset;
    border-color:rgba(110,168,254,.55);
  }
  @media (max-width:600px){
    .text-input, .select-input{ flex:1 1 200px; width:100%; }
  }

  /* ===== Buttons (compact) ===== */
  .btn{
    display:inline-flex; align-items:center; justify-content:center;
    min-height:var(--ctl-h);
    padding:0 var(--btn-pad-x);
    border-radius:var(--btn-radius);
    cursor:pointer;
    font-size:var(--fz-ctl);
    font-weight:600;
    letter-spacing:.01em;
    color:#fff;
    background: linear-gradient(180deg, var(--accent), rgba(110,168,254,0));
    border:1px solid rgba(110,168,254,.55);
    transition: box-shadow .2s ease, transform .05s ease, opacity .2s ease;
    line-height:1;                    /* avoid extra height */
  }
  .btn:hover{ box-shadow:0 0 0 2px rgba(110,168,254,.25) inset; }
  .btn:active{ transform: translateY(1px); }
  .btn.secondary{
    background: linear-gradient(180deg, var(--accent-2), rgba(114,227,166,0));
    border-color: rgba(114,227,166,.55);
  }
  .btn.ghost{
    background:transparent;
    border:1px solid var(--border-med);
    color:var(--text);
  }

  /* Save button (specific styling) */
  #saveBtn{
    position:relative;
    padding-right:36px;               /* room for icon */
    background:linear-gradient(180deg, var(--accent-2), rgba(114,227,166,0));
    border-color:rgba(114,227,166,.55);
  }
  #saveBtn::after{
    content:"💾";
    position:absolute; right:10px; top:50%;
    transform:translateY(-50%);
    font-size:12px; opacity:.9; pointer-events:none;
  }
  #saveBtn:hover{ box-shadow:0 0 0 2px rgba(114,227,166,.25) inset; }
  #saveBtn:active{ transform:translateY(1px); }
  #saveBtn:disabled{ opacity:.55; cursor:not-allowed; box-shadow:none; }

  /* ===== Viewer ===== */
  .viewer-card .body{ padding:0; }
  #glcanvas{
    width:100%; height:56vh; display:block;
    border-radius:0 0 var(--radius) var(--radius);
    border-top:1px solid var(--border-soft);
    background:
      radial-gradient(420px 220px at 18% 12%, rgba(110,168,254,.18), transparent 60%),
      #0a0f1e;
  }

  /* ===== Output + Reference layout ===== */
  .output-card .body{
    display:grid; grid-template-columns: 1fr 2fr; gap:12px; align-items:start;
  }
  @media (max-width:900px){ .output-card .body{ grid-template-columns:1fr; } }

  /* ===== Console ===== */
  #out{
    font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, "Liberation Mono", monospace;
    font-size:13px; line-height:1.4;
    white-space: pre-wrap;
    overflow-y:auto;
    max-height:240px;
    padding:10px 12px;
    background: rgba(0,0,0,0.7);
    border-radius: 12px;
    border:1px solid var(--border-soft);
  }

  /* ===== Mini Reference Dock (collapsible) ===== */
  .ref-dock{
    grid-column:2/3;
    margin-top:10px;
    border:1px solid var(--smart-border);
    border-radius:12px;
    overflow:hidden;
    background: rgba(255,255,255,0.02);
  }
  .ref-head{
    display:flex; gap:10px; align-items:center;
    padding:10px 12px; cursor:pointer; user-select:none;
    background: rgba(255,255,255,0.03);
  }
  .ref-title{
    font-weight:800; font-size:12px; letter-spacing:.08em; text-transform:uppercase; color:var(--muted);
  }
  .ref-body{ padding:12px; display:grid; gap:8px; font-size:13px; }
  .ref-dock.min .ref-body{ display:none; }
  .ref-badge{
    font:700 11px/1 Inter,system-ui,sans-serif; letter-spacing:.06em; text-transform:uppercase;
    color:white; border-radius:999px; padding:2px 8px; display:inline-block;
  }
  .badge-syntax{ background:#8b5cf6cc; }
  .badge-concept{ background:#3b82f6cc; }
  .badge-action{ background:#10b981cc; }
  .badge-warning{ background:#f59e0bcc; }

  /* ===== Layered editor (cursor aligned) ===== */
  .codewrap{
    position:relative;
    border-radius:12px;
    overflow:hidden;
    border:1px solid var(--border-strong);
    background:linear-gradient(180deg, #0f1421 0%, #0e1522 100%);
    min-height:50vh;
    scrollbar-gutter: stable both-edges;
  }
  .codewrap pre.highlighter,
  .codewrap textarea{
    font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, "Liberation Mono", monospace;
    font-size:14px;
    line-height:22px;
    font-weight:400;
    letter-spacing:0; word-spacing:0;
    tab-size:2;
    font-variant-ligatures: none;
    font-kerning: none;
    font-feature-settings: "liga" 0, "calt" 0;
    font-synthesis-weight: none;
    font-synthesis-style: none;
    -webkit-font-smoothing: antialiased;
    text-rendering: optimizeSpeed;
    padding:16px; margin:0; min-height:50vh;
  }
  .codewrap pre.highlighter{
    position:absolute; inset:0; z-index:0;
    color:#cbd5e1;
    white-space:pre;
    pointer-events:none; user-select:none; overflow:hidden; will-change: transform;
  }
  .codewrap textarea{
    position:absolute; inset:0; z-index:1;
    width:100%; height:100%;
    background:transparent; color:transparent; -webkit-text-fill-color: transparent;
    caret-color:var(--accent);
    border:none; outline:none; resize:none; overflow:auto;
    white-space:pre; overflow-wrap:normal; word-break:normal;
  }
  .wrap-on pre.highlighter, .wrap-on textarea{
    white-space:pre-wrap; word-break:break-word; overflow-wrap:anywhere;
  }

  /* Tokens */
  .tok-kw{ color:var(--green); font-weight:400 !important; }
  .tok-var{ color:var(--blue);  font-weight:400 !important; }
  .tok-rule{ color:var(--orange); font-weight:400 !important; }
  .tok-num{ color:#ff6b6b; font-weight:400 !important; }
  .tok-op{ color:#e2e8f0; font-weight:400 !important; }
  .tok-paren, .tok-brack{ color:#e7f0ff; }
  .highlighter span{ font-weight:inherit; letter-spacing:inherit; font-style:inherit; }
  .err-squiggle{
    background-image: linear-gradient(to right, transparent 0 1px, #ff7676 1px 2px, transparent 2px 3px);
    background-size: 6px 2px; background-repeat: repeat-x; background-position: 0 100%;
  }
  .ctx{ padding:1px 2px; border-radius:6px; }

  /* Scrollbars (WebKit) */
  .codewrap textarea::-webkit-scrollbar,
  .codewrap pre.highlighter::-webkit-scrollbar,
  #out::-webkit-scrollbar{ width:10px; height:10px; }
  .codewrap textarea::-webkit-scrollbar-thumb,
  .codewrap pre.highlighter::-webkit-scrollbar-thumb,
  #out::-webkit-scrollbar-thumb{ background:rgba(154,164,178,.35); border-radius:999px; }
</style>



<div class="wrap">
  <!-- ========== INPUT (Wider) ========== -->
  <section class="card input-card">
    <header>
      <h2>Grammar Input</h2>
      <div class="toolbar">
        <span id="status" aria-live="polite">Ready.</span>

        <!-- Use unique ids (avoid duplicate “nameChooser”) -->
        <input id="nameId" class="text-input" list="nameChooserList"
               placeholder="unique_name_identifier" aria-label="Type or choose identifier">
        <datalist id="nameChooserList"></datalist>

        <select id="nameChooser" class="select-input" aria-label="Choose saved identifier">
          <option value="" selected disabled>Choose saved…</option>
        </select>

        <button id="saveBtn" class="btn_save" title="SaveToJson">SaveToJson</button>
        <button id="wrapBtn" class="btn ghost" title="Toggle word wrap">⤶ Wrap: Off</button>
        <button id="go" class="btn">Run Grammar</button>
        <button id="clr" class="btn secondary">Clear</button>
      </div>
    </header>

    <div class="body">
      <div class="codewrap" id="codewrap">
        <pre id="hl" class="highlighter"></pre>
        <textarea id="src" wrap="off" spellcheck="false" placeholder="Enter Visual Grammar..."></textarea>
      </div>
    </div>
  </section>

  <!-- ========== VIEWER ========== -->
  <section class="card viewer-card">
    <header>
      <h2>Scene Viewer</h2>
      <div class="toolbar">
        <button id="orbitToggle" class="btn ghost" title="Play/Pause slow orbit">▶ Auto Orbit</button>
      </div>
    </header>
    <div class="body">
      <canvas id="glcanvas" width="1200" height="800" aria-label="WebGL scene canvas"></canvas>
    </div>
  </section>

  <!-- ========== OUTPUT + Mini Reference Dock ========== -->
  <section class="card output-card">
    <header><h2>Tokens &amp; Scene</h2></header>
    <div class="body">
      <pre id="out">Tutorial: Visual Grammar 101 — Step 0/9 (initialising)</pre>

      <div class="ref-dock min" id="refDock">
        <div class="ref-head" id="refHead">
          <span class="ref-title">Mini-Reference</span>
          <span class="ref-badge badge-syntax">Syntax</span>
          <span class="ref-badge badge-concept">Concept</span>
          <span class="ref-badge badge-action">Action</span>
          <span class="ref-badge badge-warning">Spacing</span>
          <span style="margin-left:auto; color:var(--muted); font-size:12px;" id="refToggleLabel">Show ▼</span>
        </div>
        <div class="ref-body" id="refBody">
          <div><span class="ref-badge badge-syntax">R</span> <code>R name ( min max )</code> — define a variable range (e.g., <code>R len ( 0.15 0.3 )</code>).</div>
          <div><span class="ref-badge badge-action">T</span> <code>T ( x y z )</code> — translate.</div>
          <div><span class="ref-badge badge-action">S</span> <code>S ( sx sy sz )</code> — scale (uniform or per-axis).</div>
          <div><span class="ref-badge badge-action">A</span> <code>A ( degrees 0 )</code> — rotate by degrees.</div>
          <div><span class="ref-badge badge-action">D</span> <code>D ( sx sy sz )</code> — scale cube in x/y/z (dimensions).</div>
          <div><span class="ref-badge badge-concept">I</span> <code>I ( Cube | CubeX | CubeY … )</code> — instantiate primitive.</div>
          <div><span class="ref-badge badge-syntax">Rules</span> <code>rule_name repeat vars… -> body</code>. Transform stack: <code>[</code> push, <code>]</code> pop, <code>{</code> start, <code>}</code> end.</div>
          <div><span class="ref-badge badge-warning">Spacing</span> Tokens are <strong>space-delimited</strong>. Put spaces around parentheses and brackets:<br>
            ✅ <code>S ( 1 2 3 )</code> · <code>[ Y ]</code> &nbsp; / &nbsp; ❌ <code>S(1 2 3)</code> · <code>[Y]</code></div>
        </div>
      </div>
    </div>
  </section>
</div>


Codeblocks


// Math and Grammar Space-delimited math + gramma…Code *

// Math and Grammar Space-delimited math + grammar (refactored + execute pipeline)  
"use strict";
(function(){
  const ta = document.getElementById('src');
  const hl = document.getElementById('hl');
  const wrapBtn = document.getElementById('wrapBtn');

  if (!ta || !hl) return;

  // Ensure hard wrap is truly off until toggled
  ta.setAttribute('wrap','off');

  // Scroll sync using transform (pre is overflow:hidden)
  const syncScroll = () => {
    hl.style.transform = `translate(${-ta.scrollLeft}px, ${-ta.scrollTop}px)`;
  };

  // Re-sync on events that may shift metrics
  ta.addEventListener('scroll', syncScroll, { passive:true });
  ta.addEventListener('input',  syncScroll);
  window.addEventListener('resize', syncScroll);

  // Optional wrap toggle: keep layers in lockstep
  if (wrapBtn){
    wrapBtn.addEventListener('click', () => {
      const root = document.querySelector('.wrap') || document.body;
      const on = root.classList.toggle('wrap-on');
      wrapBtn.textContent = on ? '⤶ Wrap: On' : '⤶ Wrap: Off';
      requestAnimationFrame(syncScroll);
    });
  }

  // Initial position
  syncScroll();
})();

const log2 = (...a) => {
  
    const msg = `[MATH] ${a.join(" ")}`;
    const out = (typeof document !== "undefined") && document.getElementById && document.getElementById("out");
    if (out){ out.textContent += "\n" + msg; out.scrollTop = out.scrollHeight; }
    else if (typeof console !== "undefined" && console.debug){ console.debug(msg); }
  };

/* ===== Debug switch & helper for replacevars ===== */
const ReplaceVarsDebug = (() => {
  const hasConsole = typeof console !== "undefined";
  const api = {
    enabled: false,
    buffer: [],
    log(...args) {
      if (!this.enabled) return;
      const msg = args.map(String).join(" ");
      this.buffer.push(msg);
      if (hasConsole && console.log) console.log("[replacevars]", ...args);
    },
    group(label) {
      if (!this.enabled) return;
      if (hasConsole && console.group) console.group(`[replacevars] ${label}`);
      else this.log(label);
    },
    groupEnd() {
      if (!this.enabled) return;
      if (hasConsole && console.groupEnd) console.groupEnd();
    },
    flush() {
      const out = this.buffer.join("\n");
      this.buffer.length = 0;
      return out;
    }
  };
  return api;
})();

/** Enable/disable logging for replacevars helpers. */
function setReplaceVarsDebug(on = true) {
  ReplaceVarsDebug.enabled = !!on;
}



/* ============= Minimal math (column-major) ===================== */
const Mat4 = {
  identity() { return new Float32Array([1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1]); },
  multiply(a, b) { // c = a * b
    const c = new Float32Array(16);
    for (let col = 0; col < 4; col++) {
      const bi = col * 4;
      const b0 = b[bi + 0], b1 = b[bi + 1], b2 = b[bi + 2], b3 = b[bi + 3];
      c[bi + 0] = a[0] * b0 + a[4] * b1 + a[8]  * b2 + a[12] * b3;
      c[bi + 1] = a[1] * b0 + a[5] * b1 + a[9]  * b2 + a[13] * b3;
      c[bi + 2] = a[2] * b0 + a[6] * b1 + a[10] * b2 + a[14] * b3;
      c[bi + 3] = a[3] * b0 + a[7] * b1 + a[11] * b2 + a[15] * b3;
    }
    return c;
  },
  translation(tx, ty, tz) { const m = Mat4.identity(); m[12]=tx; m[13]=ty; m[14]=tz; return m; },
  scale(sx, sy, sz)       { const m = Mat4.identity(); m[0]=sx; m[5]=sy; m[10]=sz; return m; },
  rotX(rad) { const c=Math.cos(rad), s=Math.sin(rad); return new Float32Array([1,0,0,0, 0,c,s,0, 0,-s,c,0, 0,0,0,1]); },
  rotY(rad) { const c=Math.cos(rad), s=Math.sin(rad); return new Float32Array([ c,0,-s,0, 0,1,0,0, s,0,c,0, 0,0,0,1]); },
  rotZ(rad) { const c=Math.cos(rad), s=Math.sin(rad); return new Float32Array([ c,s,0,0, -s,c,0,0, 0,0,1,0, 0,0,0,1]); }
};

const Vec3 = {
  make(x=0, y=0, z=0) { return { x, y, z }; },
  add(a, b) { return { x: a.x + b.x, y: a.y + b.y, z: a.z + b.z }; },
  mul(a, b) { return { x: a.x * b.x, y: a.y * b.y, z: a.z * b.z }; }
};



// WebGL Renderer //===========================…Code *

// WebGL Renderer 

//========================================================

(() => {
  // Triangle/edge indices for cube using vertex order 0..7
  const TRI_IDX = new Uint16Array([
    0,2,1, 0,3,2,  // back
    4,5,6, 4,6,7,  // front
    0,4,7, 0,7,3,  // left
    1,2,6, 1,6,5,  // right
    3,7,6, 3,6,2,  // top
    0,1,5, 0,5,4   // bottom
  ]);
  const EDGE_IDX = new Uint16Array([
    0,1, 1,2, 2,3, 3,0,
    4,5, 5,6, 6,7, 7,4,
    0,4, 1,5, 2,6, 3,7
  ]);

  // Minimal mat4 helpers
  const Mat4 = {
    ident() { return new Float32Array([1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1]); },
    multiply(a,b,out=new Float32Array(16)) {
      const a00=a[0],a01=a[1],a02=a[2], a03=a[3];
      const a10=a[4],a11=a[5],a12=a[6], a13=a[7];
      const a20=a[8],a21=a[9],a22=a[10],a23=a[11];
      const a30=a[12],a31=a[13],a32=a[14],a33=a[15];
      const b00=b[0],b01=b[1],b02=b[2], b03=b[3];
      const b10=b[4],b11=b[5],b12=b[6], b13=b[7];
      const b20=b[8],b21=b[9],b22=b[10],b23=b[11];
      const b30=b[12],b31=b[13],b32=b[14],b33=b[15];
      out[0]=a00*b00+a10*b01+a20*b02+a30*b03;
      out[1]=a01*b00+a11*b01+a21*b02+a31*b03;
      out[2]=a02*b00+a12*b01+a22*b02+a32*b03;
      out[3]=a03*b00+a13*b01+a23*b02+a33*b03;
      out[4]=a00*b10+a10*b11+a20*b12+a30*b13;
      out[5]=a01*b10+a11*b11+a21*b12+a31*b13;
      out[6]=a02*b10+a12*b11+a22*b12+a32*b13;
      out[7]=a03*b10+a13*b11+a23*b12+a33*b13;
      out[8]=a00*b20+a10*b21+a20*b22+a30*b23;
      out[9]=a01*b20+a11*b21+a21*b22+a31*b23;
      out[10]=a02*b20+a12*b21+a22*b22+a32*b23;
      out[11]=a03*b20+a13*b21+a23*b22+a33*b23;
      out[12]=a00*b30+a10*b31+a20*b32+a30*b33;
      out[13]=a01*b30+a11*b31+a21*b32+a31*b33;
      out[14]=a02*b30+a12*b31+a22*b32+a32*b33;
      out[15]=a03*b30+a13*b31+a23*b32+a33*b33;
      return out;
    },
    perspective(fovyRad, aspect, near, far) {
      const f = 1 / Math.tan(fovyRad/2), nf = 1/(near - far);
      const out = new Float32Array(16);
      out[0]=f/aspect; out[5]=f; out[11]=-1;
      out[10]=(far+near)*nf; out[14]=(2*far*near)*nf;
      return out;
    },
    lookAt(eye, target, up=[0,1,0]) {
      const [ex,ey,ez]=eye,[tx,ty,tz]=target;
      const zx=ex-tx, zy=ey-ty, zz=ez-tz;
      let rl = 1/Math.hypot(zx,zy,zz);
      const zxN=zx*rl, zyN=zy*rl, zzN=zz*rl;
      let xx = up[1]*zzN - up[2]*zyN;
      let xy = up[2]*zxN - up[0]*zzN;
      let xz = up[0]*zyN - up[1]*zxN;
      rl = 1/Math.hypot(xx,xy,xz); xx*=rl; xy*=rl; xz*=rl;
      const yx = zyN*xz - zzN*xy;
      const yy = zzN*xx - zxN*xz;
      const yz = zxN*xy - zyN*xx;
      const out = new Float32Array(16);
      out[0]=xx; out[1]=yx; out[2]=zxN; out[3]=0;
      out[4]=xy; out[5]=yy; out[6]=zyN; out[7]=0;
      out[8]=xz; out[9]=yz; out[10]=zzN; out[11]=0;
      out[12]=-(xx*ex+xy*ey+xz*ez);
      out[13]=-(yx*ex+yy*ey+yz*ez);
      out[14]=-(zxN*ex+zyN*ey+zzN*ez);
      out[15]=1;
      return out;
    }
  };

  // Shaders
  const VS = `
    attribute vec3 aPosition;
    attribute vec4 aColor;
    uniform mat4 uMVP;
    varying vec4 vColor;
    void main(){
      vColor = aColor;
      gl_Position = uMVP * vec4(aPosition, 1.0);
    }
  `;
  const FS = `
    precision mediump float;
    varying vec4 vColor;
    void main(){ gl_FragColor = vColor; }
  `;
  const VS_LINES = `
    attribute vec3 aPosition;
    uniform mat4 uMVP;
    void main(){ gl_Position = uMVP * vec4(aPosition, 1.0); }
  `;
  const FS_LINES = `
    precision mediump float;
    uniform vec4 uLineColor;
    void main(){ gl_FragColor = uLineColor; }
  `;

  function compile(gl, type, src){
    const s = gl.createShader(type);
    gl.shaderSource(s, src);
    gl.compileShader(s);
    if (!gl.getShaderParameter(s, gl.COMPILE_STATUS)) {
      const info = gl.getShaderInfoLog(s);
      gl.deleteShader(s);
      throw new Error("Shader compile failed:\n" + info);
    }
    return s;
  }
  function program(gl, vs, fs){
    const p = gl.createProgram();
    gl.attachShader(p, compile(gl, gl.VERTEX_SHADER, vs));
    gl.attachShader(p, compile(gl, gl.FRAGMENT_SHADER, fs));
    gl.linkProgram(p);
    if (!gl.getProgramParameter(p, gl.LINK_STATUS)) {
      const info = gl.getProgramInfoLog(p);
      gl.deleteProgram(p);
      throw new Error("Program link failed:\n" + info);
    }
    return p;
  }

  class WebGLSceneRenderer {
    /**
     * @param {HTMLCanvasElement|string} canvasOrSelector
     * @param {object} opts { wireframe?:boolean, palette?:[r,g,b,a][] }
     */
    constructor(canvasOrSelector, opts={}) {
      this.canvas = (typeof canvasOrSelector === "string")
        ? document.querySelector(canvasOrSelector)
        : canvasOrSelector;
      if (!this.canvas) throw new Error("Canvas not found.");

      this.dpr = Math.max(1, Math.min(window.devicePixelRatio||1, 2));
      const gl = this.canvas.getContext("webgl", {antialias:true, depth:true});
      if (!gl) throw new Error("WebGL not supported.");
      this.gl = gl;

      // Programs + locations
      this.prog = program(gl, VS, FS);
      this.progLoc = {
        aPosition: gl.getAttribLocation(this.prog, "aPosition"),
        aColor:    gl.getAttribLocation(this.prog, "aColor"),
        uMVP:      gl.getUniformLocation(this.prog, "uMVP")
      };
      this.lineProg = program(gl, VS_LINES, FS_LINES);
      this.lineLoc = {
        aPosition: gl.getAttribLocation(this.lineProg, "aPosition"),
        uMVP:      gl.getUniformLocation(this.lineProg, "uMVP"),
        uLineColor:gl.getUniformLocation(this.lineProg, "uLineColor")
      };

      // Buffers
      this.triPosBuf = gl.createBuffer();
      this.triColBuf = gl.createBuffer();
      this.linePosBuf = gl.createBuffer();

      // State
      this.palette = opts.palette || [
        [0.90,0.32,0.32,1], [0.32,0.90,0.48,1],
        [0.32,0.56,0.90,1], [0.94,0.78,0.26,1],
        [0.63,0.32,0.90,1], [0.90,0.47,0.75,1],
      ];
      this.wireframe = (opts.wireframe !== false);
      this.lineColor = new Float32Array([0,0,0,1]);

      // Camera
      this.target = [0,0,0];
      this.radius = 6; this.phi = Math.PI/6; this.theta = Math.PI/4;
      this.proj = Mat4.perspective(60*Math.PI/180, 1, 0.01, 100);
      this.view = Mat4.lookAt([0,0,this.radius], this.target);
      this.mvp  = Mat4.ident();

      // Controls
      this.#initControls();

      // Resize + GL state
      this.resize();
      window.addEventListener("resize", () => this.resize());
      gl.enable(gl.DEPTH_TEST);
      gl.clearColor(0.08,0.08,0.1,1);

      // Data
      this.scene = null;
      this.triCount = 0;
      this.lineCount = 0;

      // Animation loop
      this.needsRedraw = true;
      const loop = () => {
        if (this.needsRedraw) { this.render(); this.needsRedraw = false; }
        requestAnimationFrame(loop);
      };
      loop();
    }

    setWireframe(on=true){ this.wireframe = !!on; this.invalidate(); }
    setLineColor(r=0,g=0,b=0,a=1){ this.lineColor.set([r,g,b,a]); this.invalidate(); }
    setPalette(p){ this.palette = p; this.invalidate(); }
    invalidate(){ this.needsRedraw = true; }

    setScene(scene) {
      if (!scene || !scene.getAll) throw new Error("setScene expects a Scene instance.");
      this.scene = scene;
      this.#rebuildBuffers();
      this.invalidate();
    }

    resize() {
      const {canvas, gl, dpr} = this;
      const w = Math.floor((canvas.clientWidth||canvas.width) * dpr);
      const h = Math.floor((canvas.clientHeight||canvas.height) * dpr);
      if (w && h && (canvas.width !== w || canvas.height !== h)) {
        canvas.width = w; canvas.height = h;
        gl.viewport(0,0,w,h);
        this.proj = Mat4.perspective(60*Math.PI/180, w/Math.max(1,h), 0.01, 100);
        this.invalidate();
      }
    }

    #rebuildBuffers() {
      const items = this.scene ? this.scene.getAll() : [];
      if (!items.length) {
        this.triCount = 0; this.lineCount = 0;
        this.#uploadTriangles(new Float32Array(0), new Float32Array(0));
        this.#uploadLines(new Float32Array(0));
        return;
      }
      const triPositions = [];
      const triColors    = [];
      const linePositions = [];

      for (const it of items) {
        const color = this.palette[(it.texIndex|0) % this.palette.length] || [1,1,1,1];
        for (let i=0;i<TRI_IDX.length;i++){
          const v = it.verts[TRI_IDX[i]];
          triPositions.push(v[0],v[1],v[2]);
          triColors.push(color[0],color[1],color[2],color[3]);
        }
        for (let i=0;i<EDGE_IDX.length;i++){
          const v = it.verts[EDGE_IDX[i]];
          linePositions.push(v[0],v[1],v[2]);
        }
      }

      this.triCount  = triPositions.length / 3;
      this.lineCount = linePositions.length / 3;
      this.#uploadTriangles(new Float32Array(triPositions), new Float32Array(triColors));
      this.#uploadLines(new Float32Array(linePositions));
    }

    #uploadTriangles(posArray, colArray) {
      const gl = this.gl;
      gl.bindBuffer(gl.ARRAY_BUFFER, this.triPosBuf);
      gl.bufferData(gl.ARRAY_BUFFER, posArray, gl.STATIC_DRAW);
      gl.bindBuffer(gl.ARRAY_BUFFER, this.triColBuf);
      gl.bufferData(gl.ARRAY_BUFFER, colArray, gl.STATIC_DRAW);
    }
    #uploadLines(posArray) {
      const gl = this.gl;
      gl.bindBuffer(gl.ARRAY_BUFFER, this.linePosBuf);
      gl.bufferData(gl.ARRAY_BUFFER, posArray, gl.STATIC_DRAW);
    }

    #initControls() {
      const canvas = this.canvas;
      let dragging=false, panMode=false, lastX=0, lastY=0;

      canvas.addEventListener("mousedown",(e)=>{
        dragging = true;
        panMode = (e.button===2 || e.ctrlKey || e.metaKey);
        lastX=e.clientX; lastY=e.clientY; e.preventDefault();
      });
      window.addEventListener("mouseup", ()=>{ dragging=false; });
      window.addEventListener("mousemove",(e)=>{
        if (!dragging) return;
        const dx=e.clientX-lastX, dy=e.clientY-lastY;
        lastX=e.clientX; lastY=e.clientY;
        if (panMode){
          const panScale = this.radius*0.0015;
          this.target[0]-=dx*panScale; this.target[1]+=dy*panScale;
        } else {
          this.theta -= dx*0.005;
          this.phi   -= dy*0.005;
          const EPS=0.001;
          this.phi = Math.max(EPS, Math.min(Math.PI-EPS, this.phi));
        }
        this.invalidate();
      }, {passive:false});
      canvas.addEventListener("contextmenu",(e)=>e.preventDefault());
      canvas.addEventListener("wheel",(e)=>{
        const s = Math.exp(-e.deltaY*0.0015);
        this.radius = Math.max(0.5, Math.min(100, this.radius*s));
        this.invalidate();
        e.preventDefault();
      }, {passive:false});
    }

    render() {
      const gl = this.gl;
      gl.clear(gl.COLOR_BUFFER_BIT|gl.DEPTH_BUFFER_BIT);
      if (!this.triCount && !this.lineCount) return;

      const x = this.target[0] + this.radius*Math.sin(this.phi)*Math.cos(this.theta);
      const y = this.target[1] + this.radius*Math.cos(this.phi);
      const z = this.target[2] + this.radius*Math.sin(this.phi)*Math.sin(this.theta);
      this.view = Mat4.lookAt([x,y,z], this.target);
      this.mvp = Mat4.multiply(this.proj, this.view, this.mvp);

      // Triangles
      gl.useProgram(this.prog);
      gl.uniformMatrix4fv(this.progLoc.uMVP,false,this.mvp);
      gl.bindBuffer(gl.ARRAY_BUFFER, this.triPosBuf);
      gl.enableVertexAttribArray(this.progLoc.aPosition);
      gl.vertexAttribPointer(this.progLoc.aPosition,3,gl.FLOAT,false,0,0);
      gl.bindBuffer(gl.ARRAY_BUFFER, this.triColBuf);
      gl.enableVertexAttribArray(this.progLoc.aColor);
      gl.vertexAttribPointer(this.progLoc.aColor,4,gl.FLOAT,false,0,0);
      gl.drawArrays(gl.TRIANGLES,0,this.triCount);

      // Wireframe
      if (this.wireframe && this.lineCount){
        gl.depthRange(0.0,0.99);
        gl.useProgram(this.lineProg);
        gl.uniformMatrix4fv(this.lineLoc.uMVP,false,this.mvp);
        gl.uniform4fv(this.lineLoc.uLineColor,this.lineColor);
        gl.bindBuffer(gl.ARRAY_BUFFER,this.linePosBuf);
        gl.enableVertexAttribArray(this.lineLoc.aPosition);
        gl.vertexAttribPointer(this.lineLoc.aPosition,3,gl.FLOAT,false,0,0);
        gl.drawArrays(gl.LINES,0,this.lineCount);
        gl.depthRange(0.0,1.0);
      }
    }
  }

  // Expose
  window.WebGLSceneRenderer = WebGLSceneRenderer;
})();



// Scene "use strict"; // --- Cube vertices (un…Code *

// Scene 
"use strict";

// --- Cube vertices (unit cube centered at origin) ---
const cubeVerts = [
  [-0.5, -0.5, -0.5],
  [ 0.5, -0.5, -0.5],
  [ 0.5,  0.5, -0.5],
  [-0.5,  0.5, -0.5],
  [-0.5, -0.5,  0.5],
  [ 0.5, -0.5,  0.5],
  [ 0.5,  0.5,  0.5],
  [-0.5,  0.5,  0.5],
];

// --- Multiply a point by a column-major 4x4 (v' = M * [x y z 1]^T) ---
function transformPoint(M, v, w = 1) {
  const x = v[0], y = v[1], z = v[2];
  const xp = M[0]*x + M[4]*y + M[8]*z  + M[12]*w;
  const yp = M[1]*x + M[5]*y + M[9]*z  + M[13]*w;
  const zp = M[2]*x + M[6]*y + M[10]*z + M[14]*w;
  const wp = M[3]*x + M[7]*y + M[11]*z + M[15]*w; // stays 1 for affine
  return (wp !== 0 && wp !== 1) ? [xp/wp, yp/wp, zp/wp] : [xp, yp, zp];
}

class Scene {
  // items: { type, transform1, transform2, verts, flat, texIndex, arg, val }
  constructor() {
    this.items = [];
  }

  /**
   * Add a cube-like primitive with two-stage transform.
   * - Always applies transform1 to all verts.
   * - Then applies transform2 selectively based on type:
   *    Cube : if z > 0
   *    CubeX: if x > 0.5 (after transform1)
   *    CubeY: if y > 0.5 (after transform1)
   */
  add(type, transform1, transform2, texIndex = 0, arg = 0, val = 0.125) {
    let baseVerts = cubeVerts;

    if (type === "Cube") {
      const transformVerts = baseVerts.map(v => {
        return (v[2] > 0) ? transformPoint(transform2, v) : v;
      });
      const finalVerts = transformVerts.map(v => {
        return  transformPoint(transform1,v);
      });

      const flat = new Float32Array(finalVerts.flat());
      return this.#pushItem(type, transform1, transform2, finalVerts, flat, texIndex, arg, val);

    } else if (type === "CubeX") {
      const shiftedVerts = baseVerts.map(([x, y, z]) => [x + 0.5, y, z]);
      const transformVerts = shiftedVerts.map(v => {
        return (v[0] > 0.5) ? transformPoint(transform2, v) : v;
      });
      const finalVerts = transformVerts.map(v => {
        return  transformPoint(transform1, v);
      });

      const flat = new Float32Array(finalVerts.flat());
      return this.#pushItem(type, transform1, transform2, finalVerts, flat, texIndex, arg, val);

    } else if (type === "CubeY") {
      const shiftedVerts = baseVerts.map(([x, y, z]) => [x, y + 0.5, z]);
      const transformVerts = shiftedVerts.map(v => {
        return (v[1] > 0.5) ? transformPoint(transform2, v) : v;
      });
      const finalVerts = transformVerts.map(v => {
        return  transformPoint(transform1, v);
      });
      const flat = new Float32Array(finalVerts.flat()); // fixed: flatten final, not shifted
      return this.#pushItem(type, transform1, transform2, finalVerts, flat, texIndex, arg, val);

    } else {
      throw new Error(`Scene.add: unknown type "${type}"`);
    }
  }

  #pushItem(type, transform1, transform2, verts, flat, texIndex, arg, val) {
    const item = {
      type,
      transform1,
      transform2,
      verts,           // array of 8 [x,y,z]
      flat,            // Float32Array(24) flattened xyz
      texIndex: (texIndex | 0),
      arg: (Number(arg) || 0) | 0,
      val: Number.isFinite(val) ? Number(val) : 0
    };
    this.items.push(item);
    return item;
  }

  clear() { this.items.length = 0; }
  getAll() { return this.items.slice(); }
}



/*Scope==========================================…Code *

/*Scope================================================= */

//                                               Scope Class 

//         (C++ port) 
/* ======================================================= */
class Scope {
  constructor(other = null) {
    if (other instanceof Scope) {
      this.position   = Vec3.make(other.position.x, other.position.y, other.position.z);
      this.size       = Vec3.make(other.size.x, other.size.y, other.size.z);
      this.size2      = Vec3.make(other.size2.x, other.size2.y, other.size2.z);
      this.x          = Vec3.make(other.x.x, other.x.y, other.x.z);
      this.y          = Vec3.make(other.y.x, other.y.y, other.y.z);
      this.z          = Vec3.make(other.z.x, other.z.y, other.z.z);
      this.anglex     = other.anglex; this.angley = other.angley; this.anglez = other.anglez;
      this.Transform  = new Float32Array(other.Transform);
      this.Transform2 = new Float32Array(other.Transform2);
    } else {
      this.position   = Vec3.make(0, 0, 0);
      this.size       = Vec3.make(1, 1, 1);
      this.size2      = Vec3.make(1, 1, 1);
      this.x          = Vec3.make(1, 0, 0);
      this.y          = Vec3.make(0, 1, 0);
      this.z          = Vec3.make(0, 0, 1);
      this.anglex = 0; this.angley = 0; this.anglez = 0;
      this.Transform  = Mat4.identity();
      this.Transform2 = Mat4.identity();
    }
  }
  static #degToRad(deg) { return (deg * Math.PI) / 180.0; }
  static #wrap360(deg)  { let d = deg % 360; return d < 0 ? d + 360 : d; }

  T(v) {
    const M = Mat4.translation(v.x, v.y, v.z);
    this.Transform  = Mat4.multiply(this.Transform,  M);
    //this.Transform2 = Mat4.multiply(this.Transform2, M);
    this.position   = Vec3.add(this.position, v);
  }
  S(v) {
    this.size = Vec3.mul(this.size, v);
    const M = Mat4.scale(v.x, v.y, v.z);
    this.Transform  = Mat4.multiply(this.Transform,  M);
   // this.Transform2 = Mat4.multiply(this.Transform2, M);
    //this.size2 = Vec3.make(this.size.x, this.size.y, this.size.z);
  }
  D(v) {
    this.size2 = Vec3.mul(this.size2, v);
    const M = Mat4.scale(v.x, v.y, v.z);
    this.Transform2 =  M;
  }
  Rx(angleDeg) {
    this.anglex = Scope.#wrap360(angleDeg);
    const r = Scope.#degToRad(this.anglex);
    this.Transform  = Mat4.multiply(this.Transform,  Mat4.rotX(r));
    //this.Transform2 = Mat4.multiply(this.Transform2, Mat4.rotX(r));
    const c = Math.cos(r), s = Math.sin(r);
    this.x = Vec3.make(1, 0, 0);
    this.y = Vec3.make(0,  c, s);
    this.z = Vec3.make(0, -s, c);
  }
  Ry(angleDeg) {
    this.angley = Scope.#wrap360(angleDeg);
    const r = Scope.#degToRad(this.angley);
    this.Transform  = Mat4.multiply(this.Transform,  Mat4.rotY(r));
   // this.Transform2 = Mat4.multiply(this.Transform2, Mat4.rotY(r));
    const c = Math.cos(r), s = Math.sin(r);
    this.x = Vec3.make( c, 0, -s);
    this.y = Vec3.make( 0, 1,  0);
    this.z = Vec3.make( s, 0,  c);
  }
  Rz(angleDeg) {
    this.anglez = Scope.#wrap360(angleDeg);
    const r = Scope.#degToRad(this.anglez);
    this.Transform  = Mat4.multiply(this.Transform,  Mat4.rotZ(r));
   // this.Transform2 = Mat4.multiply(this.Transform2, Mat4.rotZ(r));
    const c = Math.cos(r), s = Math.sin(r);
    this.x = Vec3.make( c, s, 0);
    this.y = Vec3.make(-s, c, 0);
    this.z = Vec3.make( 0, 0, 1);
  }

  getTransform()  { return this.Transform; }
  getTransform2() { return this.Transform2; }
  getPosition()   { return Vec3.make(this.position.x, this.position.y, this.position.z); }
  setPosition(pos){ this.position = Vec3.make(pos.x, pos.y, pos.z); return this.getPosition(); }
  getSize()       { return Vec3.make(this.size.x, this.size.y, this.size.z); }
}
/* ======================================================= */


//                                        Context Class(scope stack) 


/* ======================================================= */
class Context {
  constructor() { this.stack = [ new Scope() ]; }
  current() { return this.stack[this.stack.length - 1]; }
  getCurrentScope() { return this.current(); }
  push() { this.stack.push(new Scope(this.current())); return this.current(); }
  pop()  { if (this.stack.length > 1) this.stack.pop(); return this.current(); }
  pushScope() { return this.push(); }
  popScope()  { return this.pop(); }
  newScope()  { this.stack.push(new Scope()); return this.current(); } // fresh identity
}



/* ======================================================= */

//                                         Error Functions

/* ======================================================= */


class SolutionError extends Error {
  constructor(message, idx = -1, expr = "") {
    const where = (idx >= 0 && expr)
      ? ` at col ${idx + 1}:\n${expr}\n${" ".repeat(idx)}^`
      : "";
    super(`${message}${where}`); this.name = "SolutionError";
  }
}
class ParseError extends Error {
  constructor(message, line = 0, col = 0, snippet = "") {
    super(message); this.name = "ParseError"; this.line = line; this.col = col; this.snippet = snippet;
  }
}
class RuntimeGrammarError extends Error { constructor(message){ super(message); this.name="RuntimeGrammarError"; } }
const ErrorReporter = {
  caret(line, col) { return `${line}\n${" ".repeat(Math.max(0, col))}^`; },
  fromToken(msg, tok) { return new ParseError(msg, tok.line, tok.col, this.caret(tok.sourceLine, tok.col)); }
};
function errorout(e) {
  const hasConsole = typeof console !== "undefined";
  if (e instanceof ParseError) {
    if (hasConsole && console.warn) console.warn(`[ParseError] ${e.message} (line ${e.line}, col ${e.col})\n${e.snippet}`);
  } else {
    if (hasConsole && console.warn) console.warn(String(e));
  }
}



/*Variable=======================================…Code *

/*Variable========================================== */

//                                                           Variable Class

/* ======================================================= */
const variable_list = [];
const full_variable_list = [];

class Variable {
  constructor(name, min, max, i) {
    if (typeof max==="undefined" && typeof i==="undefined") {
      this.var_name=name; this.min=0; this.max=0; this.value=min; this.instance_count=0; this.integer=false; return;
    }
    this.var_name=name; this.min=min; this.max=max; this.integer=!!i; this.instance_count=0;
    const u=RNG.random();
    this.value = this.integer
      ? Math.floor(u*(Math.floor(max)-Math.ceil(min)+1))+Math.ceil(min)
      : u*(max-min)+min;
  }
  getRandom() {
    const u=RNG.random();
    return this.integer
      ? Math.floor(u*(Math.floor(this.max)-Math.ceil(this.min)+1))+Math.ceil(this.min)
      : u*(this.max-this.min)+this.min;
  }
}
function findVariableForward(var_name){ for (let i=0;i<variable_list.length;i++) if (variable_list[i].var_name===var_name) return i; return -1; }
function removeVariable(var_name){
  for (let i=variable_list.length-1;i>=0;i--){
    if (variable_list[i].var_name===var_name && variable_list[i].max===variable_list[i].min) { variable_list.splice(i,1); return; }
  }
}
function addVariable(var_name,min,max,integer){
  if (findVariableForward(var_name)===-1){
    const v=new Variable(var_name,min,max,integer); v.value=v.getRandom();
    variable_list.push(v); full_variable_list.push(v);
//log2(`Adding variable ${var_name}.`);
  }
}
function addVariableInstance(var_name){
  const idx=findVariableForward(var_name); if (idx!==-1){

    const ref=variable_list[idx], v=new Variable(var_name, ref.getRandom());
    v.instance_count=ref.instance_count+1; variable_list.push(v); full_variable_list.push(v);
//log2(`Adding variable instance ${var_name}.`);
  }
}



/* ==============================================…Code *

/* ================================================== */

//                                        Substitution (instrumented) 

/* ================================================== */
function replacevars(s) {
//setReplaceVarsDebug(true);
  s = String(s);
//  log2(`replacevars IN="${s}"`);
  for (let i = variable_list.length - 1; i >= 0; i--) {
    const v = variable_list[i];
    if (v.max === v.min) {
      const name = v.var_name;
      const pos = s.indexOf(name);
  //   log2(`scan[${i}] name=${name} value=${v.value} int=${!!v.integer} pos=${pos}`);
      if (pos !== -1) {
        const before = s;
        s = s.slice(0, pos) + String(v.value) + s.slice(pos + name.length);
   //     log2(`replaced "${name}" at ${pos}:`, `before="${before}"`, `after="${s}"`);
        break;
      }
    }
  }
  ReplaceVarsDebug.groupEnd();
  return s;
}


function replacevars_ampersand(s) {
  s = String(s);
  ReplaceVarsDebug.group(`replacevars_ampersand IN="${s}"`);
  for (let i = 0; i < variable_list.length; i++) {
    const v = variable_list[i];
    const name = v.var_name;
    const tag = "&" + name;
    const pos = s.indexOf(tag);
    ReplaceVarsDebug.log(`scan[${i}] tag=${tag} pos=${pos}`);
    if (pos !== -1) {
      const rnd = v.getRandom();
      const before = s;
      s = s.slice(0, pos) + String(rnd) + s.slice(pos + tag.length);
      ReplaceVarsDebug.log(`replaced "${tag}" at ${pos} with ${rnd}:`, `before="${before}"`, `after="${s}"`);
      break;
    }
  }
  ReplaceVarsDebug.groupEnd();
  return s;
}

/* -------- Robust evaluation that replaces ALL identifiers with boundaries -------- */
function MathS2_js(input){
  const X = new Solution();
  let s = String(input);

s=replacevars(replacevars(replacevars(replacevars(replacevars(replacevars( replacevars(s)))))));

  const out = X.Process(s);
  if (!Number.isFinite(out)) {
    if (typeof console !== "undefined" && console.warn) {
      console.warn("[MathS2_js] Non-finite evaluation:", { input, substituted: s, out });
    }
    return 0;
  }
  return out; // return NUMBER
}



/* ================= RNG (guard zero state) =====…Code *

/* ================= RNG (guard zero state) ====================== */
const RNG = (() => {
  const UINT32 = 0x100000000;
  const GOLDEN = 0x9e3779b9 | 0;
  let state = ((0x12345678 ^ Date.now()) | 0) ^ GOLDEN;
  const nz = x => ((x|0) === 0 ? GOLDEN : (x|0));
  state = nz(state);
  function xorshift32() {
    let x = state | 0;
    x ^= x << 13; x ^= x >>> 17; x ^= x << 5;
    state = nz(x);
    return (x >>> 0) / UINT32;
  }
  return {
    random() { return xorshift32(); },
    setSeed(seed) { const s = Number.isFinite(seed) ? (seed >>> 0) : (Date.now() >>> 0); state = nz(((s ^ GOLDEN) | 0)); }
  };
})();

/* ====================== Math evaluator ====================== */
class Solution {
  constructor() {
    this.precedence = { "+":1, "-":1, "*":2, "/":2, "^":3 };
    this.rightAssoc = { "^": true };
    this.functions = new Set(["sin","cos"]);
    this.eps = 1e-6;
  }
  Process(input) {
    const expr = String(input ?? "").trim();
    if (!expr) return 0;
    try {
      const tokens = this.#tokenize(expr);
      const rpn = this.#toRPN(tokens, expr);
      let value = this.#evalRPN(rpn, expr);
      if (Math.abs(value) < this.eps) value = 0;
      return value;
    } catch (e) {
      if (typeof console !== "undefined" && console.warn) {
        console.warn(e instanceof SolutionError ? e.message : String(e));
      }
      return 0;
    }
  }
  #tokenize(expr) {
  const out = [],
        isD = c => c >= "0" && c <= "9",
        isA = c => /[A-Za-z_]/.test(c),
        isAB= c => /[A-Za-z0-9_]/.test(c);

  let i = 0, prev = "START";
  const n = expr.length;

  while (i < n) {
    const ch = expr[i];

    // whitespace
    if (/\s/.test(ch)) { i++; continue; }



    // numbers (incl. .5)
    if (isD(ch) || (ch === "." && isD(expr[i+1] || ""))) {
      const s = i;
      i = this.#readNum(expr, i);
      const v = Number(expr.slice(s, i));
      if (!Number.isFinite(v)) throw new SolutionError("Non-finite numeric literal", s, expr);
      out.push({ type:"num", value:v, idx:s });
      prev = "num";
      continue;
    }

    // identifiers
    if (isA(ch)) {
      const s = i;
      i++;
      while (i < n && isAB(expr[i])) i++;
      out.push({ type:"id", value:expr.slice(s, i), idx:s });
      prev = "id";
      continue;
    }

    // parens
    if (ch === "(" || ch === ")") {
      out.push({ type:"paren", value:ch, idx:i });
      i++;
      prev = (ch === "(") ? "lparen" : "rparen";
      continue;
    }

    // operators
    if (ch==="+" || ch==="-" ||  ch==="*" || ch==="/" || ch==="^" ){
      const s = i;
      const unary = ((ch === "+" || ch === "-") && (prev === "START" || prev === "op" || prev === "lparen"));
      if (unary) {
        const sign = ch, next = expr[i+1] || "";
        // signed number
        if (isD(next) || (next === "." && isD(expr[i+2] || ""))) {
          i++;
          const ns = i;
          i = this.#readNum(expr, i);
          const v = Number(sign + expr.slice(ns, i));
          if (!Number.isFinite(v)) throw new SolutionError("Malformed signed number", s, expr);
          out.push({ type:"num", value:v, idx:s });
          prev = "num";
          continue;
        }
        // unary +/- before ( or id  => rewrite as 0 +/- <expr>
        if (next === "(" || /[A-Za-z_]/.test(next)) {
          out.push({ type:"num", value:0, idx:s });
          out.push({ type:"op", value:(sign === '-') ? '-' : '+', idx:s });
          i++;
          prev = "op";
          continue;
        }
        throw new SolutionError(`Dangling unary '${ch}'`, s, expr);
      }
      // binary operator
      out.push({ type:"op", value:ch, idx:s });
      i++;
      prev = "op";
      continue;
    }

    // otherwise
    throw new SolutionError(`+++Unexpected character '${ch}'`, i, expr);
  }

  // Turn id + '(' into func
  for (let k = 0; k < out.length - 1; k++) {
    if (out[k].type === "id" && out[k+1].type === "paren" && out[k+1].value === "(") {
      if (!this.functions.has(out[k].value)) {
        throw new SolutionError(`Unknown function '${out[k].value}'`, out[k].idx, expr);
      }
      out[k].type = "func";
    }
  }

  return out;
}
  #readNum(expr,i){
    const isD=c=>c>="0"&&c<="9"; let j=i; while(isD(expr[j])) j++;
    if (expr[j]==="."){ j++; while(isD(expr[j])) j++; }
    if (expr[j]==="e"||expr[j]==="E"){ j++; if (expr[j]==="+"||expr[j]==="-") j++; if (!isD(expr[j])) throw new SolutionError("Malformed exponent", j, expr); while(isD(expr[j])) j++; }
    return j;
  }
  #toRPN(tokens,expr){
    const out=[], st=[];
    for (const t of tokens){
      if (t.type==="num") { out.push(t); continue; }
      if (t.type==="id")  throw new SolutionError(`Unknown identifier '${t.value}'`, t.idx, expr);
      if (t.type==="func"){ st.push(t); continue; }
      if (t.type==="op") {
        const p1=this.precedence[t.value]??-1, right=!!this.rightAssoc[t.value];
        while(st.length&&st[st.length-1].type==="op"){
          const p2=this.precedence[st[st.length-1].value]??-1;
          if ((right&&p1<p2)||(!right&&p1<=p2)) out.push(st.pop()); else break;
        }
        st.push(t); continue;
      }
      if (t.type==="paren" && t.value==="(") { st.push(t); continue; }
      if (t.type==="paren" && t.value===")") {
        let ok=false; while(st.length){ const top=st.pop(); if (top.type==="paren"&&top.value==="("){ ok=true; break; } out.push(top); }
        if (!ok) throw new SolutionError("Mismatched ')'", t.idx, expr);
        if (st.length && st[st.length-1].type==="func") out.push(st.pop()); continue;
      }
      throw new SolutionError(`Unknown token '${t.value}'`, t.idx, expr);
    }
    while(st.length){ const top=st.pop(); if (top.type==="paren") throw new SolutionError("Mismatched '('", top.idx, expr); out.push(top); }
    return out;
  }



  #evalRPN(rpn, expr){
    const st=[], need=(n,idx,what)=>{ if (st.length<n) throw new SolutionError(`Not enough operands for ${what}`, idx, expr); };
    for (const t of rpn){
      if (t.type==="num"){ st.push(t.value); continue; }
      if (t.type==="op"){
        need(2, t.idx, `'${t.value}'`);
        const b=st.pop(), a=st.pop();
        let v; switch(t.value){
          case "+": v=a+b; break; case "-": v=a-b; break; case "*": v=a*b; break;
          case "/": if (b===0) throw new SolutionError("Division by zero", t.idx, expr); v=a/b; break;
          case "^": v=Math.pow(a,b); break; default: throw new SolutionError(`Unknown operator '${t.value}'`, t.idx, expr);
        }
        if (!Number.isFinite(v)) throw new SolutionError(`Non-finite result for '${t.value}'`, t.idx, expr);
        st.push(v); continue;
      }
      if (t.type==="func"){
        need(1, t.idx, t.value); const x=st.pop();
        let v = (t.value==="sin") ? Math.sin(x) : Math.cos(x);
        if (!Number.isFinite(v)) throw new SolutionError(`Non-finite result for ${t.value}`, t.idx, expr);
        st.push(v); continue;
      }
      throw new SolutionError(`Bad RPN token '${t.value}'`, t.idx, expr);
    }
    if (st.length!==1) throw new SolutionError("Expression did not reduce to a single value");
    return st[0];
  }
}

/* ====================== Utilities ====================== */
function breakup(input, delimiter) {
  const out=[]; let s=String(input); let pos;
  while ((pos=s.indexOf(delimiter))!==-1) { out.push(s.substring(0,pos)); s=s.substring(pos+delimiter.length); }
  out.push(s); return out;
}



// Grammar Lexer (keeps WS) — includes MATH and s…Code *

// Grammar Lexer (keeps WS) — includes MATH and strict '->' (no whitespace), no RegExp usage anywhere
class GrammarLexer {
  constructor(text){
    this.src = String(text);
    this.tokens = [];
    this._lex();
  }

  _push(type, value, line, col, lineStart){
    const lineEnd = this.src.indexOf("\n", lineStart);
    const sourceLine = this.src.slice(lineStart, lineEnd === -1 ? this.src.length : lineEnd);
    this.tokens.push({ type, value, line, col, sourceLine });
  }

  // ===== character helpers (no regex) =====
  _isSpace(c){ return c===" "||c==="\t"||c==="\r"||c==="\v"||c==="\f"; }
  _isDigit(c){ return c>="0" && c<="9"; }
  _isLetter(c){ return (c>="A"&&c<="Z")||(c>="a"&&c<="z")||c==="_"; }
  _isIdentC(c){ return this._isLetter(c)||this._isDigit(c); }

  // Read an unsigned number starting at i. Returns end index or i if not a number.
  _readNumber(s, i, end){
    let j = i, has = false;
    while (j < end && this._isDigit(s[j])) { j++; has = true; }
    if (j < end && s[j] === "."){
      let k = j + 1, any = false;
      while (k < end && this._isDigit(s[k])) { k++; any = true; }
      if (any){ j = k; has = true; }
    }
    if (!has) return i;
    if (j < end && (s[j] === "e" || s[j] === "E")){
      let k = j + 1;
      if (k < end && (s[k] === "+" || s[k] === "-")) k++;
      let any = false;
      while (k < end && this._isDigit(s[k])) { k++; any = true; }
      if (!any) return i;
      j = k;
    }
    return j;
  }

  // Read a signed number starting at i (optional +/-). Returns end index or i if not a (signed) number.
  _readSignedNumber(s, i, end){
    let p = i;
    if (p < end && (s[p] === "+" || s[p] === "-")) p++;
    const j = this._readNumber(s, p, end);
    return (j > p) ? j : i;
  }

  // Read identifier with optional prefix & and optional leading +/- (mirrors prior ID rule).
  _readIdentifier(s, i, end){
    let j = i;
    if (j < end && s[j] === "&") j++;
    if (j < end && (s[j] === "+" || s[j] === "-")) j++;
    if (!(j < end && this._isLetter(s[j]))) return i;
    j++;
    while (j < end && this._isIdentC(s[j])) j++;
    return j;
  }

// Try to match one self-contained MATH span starting at `start` (no regex),
// spaces are NOT allowed once the span begins. Includes debug output.
_matchMath(start){
  const DEBUG = false; // flip to false to disable logging

  const log = (...a) => {
    if (!DEBUG) return;
    const msg = `[MATH] ${a.join(" ")}`;
    const out = (typeof document !== "undefined") && document.getElementById && document.getElementById("out");
    if (out){ out.textContent += "\n" + msg; out.scrollTop = out.scrollHeight; }
    else if (typeof console !== "undefined" && console.debug){ console.debug(msg); }
  };

  const s = this.src, len = s.length;
  let pos = start;

  // helpers (no regex)
  const isSpace  = this._isSpace.bind(this);
  const isDigit  = this._isDigit.bind(this);
  const isLetter = this._isLetter.bind(this);
  const isIdentC = this._isIdentC.bind(this);
  const isOpOrPar = c => c==="("||c===")"||c==="+"||c==="-"||c==="*"||c==="/"||c==="^";

  // skip leading spaces before the span
  while (pos < len && isSpace(s[pos])) pos++;

  log(`BEGIN start=${start} first='${s[pos]??""}'`);

  let sawToken = false;
  let paren = 0;
  let hasBinaryOp = false;
  let hasFuncCall = false;
  let lastWasOperand = false; // operand = number | identifier | ')'

  while (pos < len){
    const ch = s[pos];

    // hard stops
    if (ch === "\n" || ch === "#"){ log(`BREAK at '${ch}' pos=${pos}`); break; }

    // once we've started, a space ends the MATH span
    if (isSpace(ch)){
      if (!sawToken) { pos++; continue; }        // still before first token
      log(`BREAK on space inside span at pos=${pos}`);
      break;
    }

    const tStart = pos;

    // 1) operator / paren
    if (isOpOrPar(ch)){
      pos++; sawToken = true;

      if (ch === "("){ paren++; lastWasOperand = false; log(`'(' depth=${paren}`); continue; }
      if (ch === ")"){
        if (paren === 0){ log(`UNMATCHED ')' -> back to ${tStart}`); pos = tStart; break; }
        paren--; lastWasOperand = true; log(`')' depth=${paren}`); continue;
      }
      if (ch === "-"){
        const unary = !lastWasOperand;
        if (!unary){ hasBinaryOp = true; log(`'-' binary`); }
        else { log(`'-' unary`); }
        lastWasOperand = false; continue;
      }
      // + * / ^ are binary iff preceded by operand
      if (lastWasOperand){ hasBinaryOp = true; log(`'${ch}' binary`); }
      else { log(`'${ch}' leading`); }
      lastWasOperand = false; continue;
    }

    // 2) number (.5, exponent). Use existing helper.
    if (isDigit(ch) || (ch === "." && isDigit(s[pos+1]||""))){
      const j = this._readNumber(s, pos, len);
      if (j > pos){
        log(`num '${s.slice(pos,j)}'`);
        pos = j; sawToken = true; lastWasOperand = true;
        continue;
      }
    }

    // 3) identifier (variable or function). No spaces allowed between func and '('
    if (isLetter(ch)){
      let j = pos + 1;
      while (j < len && isIdentC(s[j])) j++;
      const name = s.slice(pos, j);

      // accept builtin funcs or known variables (user helper findVariableForward expected)
      const isFunc = (name === "sin" || name === "cos");
      const isVar  = typeof findVariableForward === "function" ? (findVariableForward(name) !== -1) : true;

      if (!(isFunc || isVar)){ log(`STOP invalid ident '${name}'`); break; }

      if (isFunc){
        // must be an immediate '(' (no spaces allowed inside span)
        if (s[j] === "("){ hasFuncCall = true; log(`func '${name}('`); }
        else { log(`func '${name}' without '(' -> treat as ident`); }
      } else {
        log(`ident '${name}'`);
      }

      pos = j; sawToken = true; lastWasOperand = true;
      continue;
    }

    // anything else ends the span
    log(`STOP other '${ch}' at ${pos}`);
    break;
  }

  if (!sawToken){ log(`NO_TOKEN`); return null; }

  const end = pos;
  const raw = s.slice(start, end);
  log(`RAW='${raw}' depth=${paren} lastOpnd=${lastWasOperand} hasOp=${hasBinaryOp} hasCall=${hasFuncCall}`);

  // validity checks
  if (paren !== 0){ log(`REJECT unbalanced depth=${paren}`); return null; }

  // reject pure signed number (use existing number reader on trimmed raw)
  const isPureSignedNumber = (str)=>{
    let a = 0, b = str.length;
    while (a < b && isSpace(str[a])) a++;
    while (b > a && isSpace(str[b-1])) b--;
    if (a >= b) return false;
    let p = a;
    if (str[p] === "+" || str[p] === "-") p++;
    const q = this._readNumber(str, p, b);
    while (q < b && isSpace(str[q])) return false; // internal spaces would have broken earlier
    return (q > p && q === b);
  };
  if (isPureSignedNumber(raw)){ log(`REJECT pure number`); return null; }

  if (!lastWasOperand){ log(`REJECT ends with operator`); return null; }
  if (!(hasBinaryOp || hasFuncCall)){ log(`REJECT no op/call`); return null; }

  log(`ACCEPT [${start},${end}) '${raw}'`);
  return { start, end, text: raw };
}

  // Match '->' (no spaces allowed)
  _matchArrow(i){
    const s = this.src;
    if (i + 1 < s.length && s[i] === "-" && s[i + 1] === ">"){
      return { end: i + 2 };
    }
    return null;
  }

  _lex(){
    const s = this.src;
    const isSpace = this._isSpace.bind(this);
    let i = 0, line = 1, col = 0, lineStart = 0;

    // single-char operators/delims used by the grammar (outside MATH).
    // NOTE: include "/" so lone division outside MATH is treated as OP, not an error.
    const singles = new Set(["(",")",";","|","[","]","{","}","*","/"]);

    while (i < s.length){
      const c = s[i];

      // Newline
      if (c === "\n"){ i++; line++; col = 0; lineStart = i; continue; }

      // Whitespace (not newline)
      if (isSpace(c)){
        let j = i + 1;
        while (j < s.length && isSpace(s[j]) && s[j] !== "\n") j++;
        //this._push("WS", s.slice(i, j), line, col, lineStart);
        col += (j - i); i = j; continue;
      }

      // Comments
      if (c === "#"){                          // shell-style to end of line
        const j = s.indexOf("\n", i);
        const end = j === -1 ? s.length : j;
        //this._push("CMT", s.slice(i, end), line, col, lineStart);
        col += (end - i); i = end; continue;
      }
      if (c === "/" && s[i+1] === "/"){        // C++-style line comment
        let j = i + 2;
        while (j < s.length && s[j] !== "\n") j++;
       // this._push("CMT", s.slice(i, j), line, col, lineStart);
        col += (j - i); i = j; continue;
      }
      if (c === "/" && s[i+1] === "*"){        // C-style block comment
        let j = i + 2, closed = false;
        while (j < s.length){
          if (s[j] === "\n"){ line++; col = 0; lineStart = j + 1; }
          if (s[j] === "*" && s[j+1] === "/"){ j += 2; closed = true; break; }
          j++;
        }
        if (!closed){
          const lineEnd = s.indexOf("\n", lineStart);
          const errLine = s.slice(lineStart, lineEnd === -1 ? s.length : lineEnd);
          throw new ParseError("Unterminated block comment", line, col, ErrorReporter.caret(errLine, col));
        }
        //this._push("CMT", s.slice(i, j), line, col, lineStart);
        i = j; col = i - lineStart;            // reset column relative to current line
        continue;
      }

      // Arrow
      const arr = this._matchArrow(i);
      if (arr){
        this._push("OP", "->", line, col, lineStart);
        col += (arr.end - i); i = arr.end; continue;
      }

      // MATH (before OP/ID/NUM)
      const math = this._matchMath(i);
      if (math){
        this._push("MATH", math.text, line, col, lineStart);
        col += (math.end - i); i = math.end; continue;
      }

      // Single-char operators / delimiters (now includes '/')
      if (singles.has(c)){ this._push("OP", c, line, col, lineStart); i++; col++; continue; }

      // Identifier (no regex): ^&?[+-]?[A-Za-z_][A-Za-z0-9_]*
      {
        const end = this._readIdentifier(s, i, s.length);
        if (end > i){
          this._push("ID", s.slice(i, end), line, col, lineStart);
          col += (end - i); i = end; continue;
        }
      }

      // Number (no regex): ^[-+]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?  (handled by helpers)
      {
        const end = this._readSignedNumber(s, i, s.length);
        if (end > i){
          this._push("NUM", s.slice(i, end), line, col, lineStart);
          col += (end - i); i = end; continue;
        }
      }

      // Unexpected
      const lineEnd = s.indexOf("\n", lineStart);
      const errLine = s.slice(lineStart, lineEnd === -1 ? s.length : lineEnd);
      throw new ParseError(`***Unexpected character '${c}'`, line, col, ErrorReporter.caret(errLine, col));
    }
  }
}


/* ==================================================== */

//                                                   Token Class

/* ===================================================== */
class Token {
  static fromToken(t){ const nt=new Token(t.token_name);
    nt.arguments=[...t.arguments]; nt.var_names=[...t.var_names];
    nt.var_name=t.var_name; nt.instance_type=t.instance_type;
    nt.instance_count=t.instance_count|0; nt.integer=!!t.integer; return nt;
  }
  constructor(name, instCount=0){
    this.token_name=name||""; this.arguments=[]; this.instance_type=""; this.instance_count=instCount|0;
    this.var_name=""; this.var_names=["","",""]; this.integer=false;
  }
  addArgument(v){ const n=+v; if (!Number.isFinite(n)) throw new RuntimeGrammarError(`Non-finite token argument for ${this.token_name}`); this.arguments.push(n); }
  addInstanceType(s){ this.instance_type=String(s); }
  isRule(){
    if (this.token_name!=="{" && this.token_name!=="}" && this.token_name!=="+" &&
        this.token_name!=="*" && this.token_name!=="[" && this.token_name!=="]" &&
        this.instance_type==="" && this.arguments.length===0) return this.token_name;
    return "";
  }
  setVarName(s) { this.var_name = String(s); }
  print(){
    const fmt=v=>{ let s=Number(v).toFixed(5); if (s.endsWith(".00000")) s=s.slice(0,-6); return s+" "; };
    let ss=(this.token_name==="R" && this.integer) ? "R* " : (this.token_name+" ");
    if (this.token_name==="[" || this.token_name==="]" || this.isRule()!=="") return ss;
    if (this.var_name) ss+=this.var_name+" ";
    ss+="( "; if (this.instance_type) ss+=this.instance_type+" ";
    ss+= (this.var_names[0] || fmt(this.arguments[0] ?? 0));
    ss+= (this.var_names[1] || fmt(this.arguments[1] ?? 0));
    if (this.arguments.length>2 || this.var_names[2]) ss+= (this.var_names[2] || fmt(this.arguments[2] ?? 0));
    ss+=") "; return ss;
  }
//======================================================

//                                         Perform Action
  /* ---------- Execute token against context + scene (now uses MathS2_js) ---------- */

//=======================================================
  performAction(context, scene, deps = {}) {
    const { addVariable = () => {} } = deps;
    const numArg = (i) => Number(this.arguments[i] ?? 0);
    const evalMaybe = (name, fallback) => (name ? Number(MathS2_js(name)) : Number(fallback));

    switch (this.token_name) {
      case "R": {
        addVariable(this.var_name, numArg(0), numArg(1), !!this.integer);
        addVariableInstance(this.var_name, numArg(0), numArg(1), !!this.integer);
        return;
      }
      case "S":
      case "D":
      case "T": {
        const s = context.getCurrentScope();
        const v = {
          x: evalMaybe(this.var_names[0], numArg(0)),
          y: evalMaybe(this.var_names[1], numArg(1)),
          z: evalMaybe(this.var_names[2], numArg(2)),
        };
        if (this.token_name === "S") s.S(v);
        else if (this.token_name === "D") s.D(v);
        else s.T(v);
        return;
      }
      case "A": {
        const s = context.getCurrentScope();
        const angle = evalMaybe(this.var_names[0], numArg(0));
        const axis  = (evalMaybe(this.var_names[1], numArg(1))|0);
        if (axis === 0) s.Rx(angle); else if (axis === 1) s.Ry(angle); else s.Rz(angle);
        return;
      }
      case "I": {
        const transform1 = context.getCurrentScope().getTransform();
        const transform2 = context.getCurrentScope().getTransform2();
        const _texIndex = this.var_names[0] ? (Number(MathS2_js(this.var_names[0])) | 0) : (numArg(0) | 0);
        const arg = (this.arguments.length > 1) ? (this.arguments[1] | 0) : 0;
        const val = (this.arguments.length > 2) ? Number(this.arguments[2]) : 0.125;
       

//////////////////////////////////////////////////////////
        scene.add(this.instance_type,transform1,transform2, _texIndex,arg, val);  ////////////SCENE
        

        return;
      }
      case "[": context.pushScope(); return;
      case "]": context.popScope();  return;
      case "{": context.newScope();  return;
      case "}": context.popScope();  return;
      default:  return; // rule name, no-op here
    }
  }
}
/* ======================================================= */

//                                    Rule Class

/* ======================================================= */
class Rule {
  constructor(name, repeat){
    this.rule_name=name; this.repeat=repeat; this.count=0;
    this.tokens=[]; this.section_tokens=[[],[],[]];
    this.var_name=""; this.var_names=Array.from({length:20},()=>"" );
    this.var_counter=0; this.probability=1.0; this.alternate=null;
  }
  addToken(tok, sec){
    if (!Array.isArray(this.section_tokens[sec])) this.section_tokens[sec]=[];
    this.section_tokens[sec].push(tok);
    if (sec===1) this.tokens.push(tok);
  }
  print(){
    let ss=this.rule_name+" ";
    ss+= (this.var_name!=="") ? (this.var_name+" ") : (this.repeat+" ");
    for (let i=0;i<this.var_counter;i++) if (this.var_names[i] !== "") ss+=this.var_names[i]+" ";
    if (this.probability<1.0) ss+= "; "+this.probability+" ";
    ss+="-> ";
    for (let k=0;k<3;k++){
      for (const t of this.section_tokens[k]) ss+=t.print();
      if (k===0 && this.section_tokens[0].length) ss+="| ";
      if (k===1 && this.section_tokens[2].length) ss+="| ";
    }
    if (this.alternate){
      ss+="-> ";
      for (const t of this.alternate.section_tokens[1]) ss+=t.print();
    }
    return ss;
  }
}



//===============================================…Code *

//=====================================================


//                                                  Grammar Class


//====================================================


class Grammar {
  constructor(fileText){
    if (typeof fileText!=="string") throw new ParseError("Input must be a string");
    this.rule_list=[]; this.tokens_new=[];

    const lines=String(fileText).split(/\r?\n/);
    for (let idx=0; idx<lines.length; idx++){
      const line=lines[idx];
      if (!line || line.trim()==="" || line.trim().startsWith("#")) continue;

      const parts=breakup(line,"->");
      const header=(parts[0]||"")+" ->";
      const stream=this.makeStream(header);

      const name = stream.nextVal();
      if (!name) throw new ParseError("Missing rule name", idx+1, 0, line);

      const rule=new Rule(name, 1);
      this.#ruleHeader(rule, stream, parts[1]||"", idx, line);
      if (parts.length>2) this.#ruleAlternate(rule, parts[2]||"");
      this.rule_list.push(rule);
    }
    if (this.rule_list.length===0) throw new ParseError("No rules parsed. Ensure lines are 'RuleName -> ...'");

    this.tokens_new=this.Recurse(this.rule_list[0]);
  }

  #ruleAlternate(rule, s){
    rule.alternate=new Rule(rule.rule_name+"_Alt",1);
    rule.alternate.probability=Math.max(0, Math.min(1, 1.0 - rule.probability));
    this.ReadTokens(rule.alternate, s, 1);
  }

  #ruleHeader(rule, stream, body, idx, rawLine){
    let tok=stream.nextVal();
    if (tok === ";") {
      const p=stream.nextVal(); if (!this.isNumStr(p)) throw new ParseError("Expected probability after ';'", idx+1, 0, rawLine);
      rule.probability=parseFloat(p);
    } else {
      if (tok !== "->") { if (!this.isNumStr(tok)) rule.var_name=tok; else rule.repeat=parseInt(tok,10); }
      while (true){
        tok=stream.nextVal();
        if (tok===null || tok===";" || tok==="->") break;
        if (findVariableForward(tok)!==-1){ rule.var_names[rule.var_counter]=tok; rule.var_counter++; }
      }
      if (tok === ";") {
        const p=stream.nextVal(); if (!this.isNumStr(p)) throw new ParseError("Expected numeric probability after ';'", idx+1, 0, rawLine);
        rule.probability=parseFloat(p);
      }
    }

    const sections=breakup(body,"|");
    let n=sections.length; if (body.length && body[body.length-1]==="|") n--;
    if (n>3) throw new ParseError("At most 3 sections allowed (pre|main|post)", idx+1, 0, rawLine);
    if (n===1) this.ReadTokens(rule, body, 1); else for (let i=0;i<n;i++) this.ReadTokens(rule, sections[i], i);
  }

//========================================================

//                                Read Tokens

//========================================================
 
 ReadTokens(rule, rule_str, sec){
    const L = new GrammarLexer(rule_str);
    const S = this.makeStreamFromLexer(L);
  
const log = (...a) => {
    if (!DEBUG) return;
    const msg = `[MATH] ${a.join(" ")}`;
    const out = (typeof document !== "undefined") && document.getElementById && document.getElementById("out");
    if (out){ out.textContent += "\n" + msg; out.scrollTop = out.scrollHeight; }
    else if (typeof console !== "undefined" && console.debug){ console.debug(msg); }
  };

    try {
      while (true){
        const t = S.nextTok(); if (!t) break;
        if (t.type==="WS" || t.type==="CMT") continue;

        // S / T / D
        if (t.type==="ID" && (t.value==="S"||t.value==="T"||t.value==="D")){
          const name=t.value; this.expectOp(S,"(", name);
          const tok=new Token(name);
          for (let i=0;i<3;i++){
            const a=S.nextTokReq(`${name} missing argument ${i}`);
            if (a.type==="NUM") tok.addArgument(parseFloat(a.value));
            else if (a.type==="ID") { tok.var_names[i]=a.value; tok.addArgument(0); }
            else if (a.type==="MATH"){const arg=parseFloat(MathS2_js(a.value)); tok.addArgument(arg); log(`output=${arg} input=${a.value} token : ${name}`);}
            else throw ErrorReporter.fromToken(`${name}: argument ${i} must be number or identifier`, a);
          }
          this.expectOp(S,")", name);
          rule.addToken(tok, sec); continue;
        }

        // R / R*
        if (t.type==="ID" && (t.value==="R" || t.value==="R*")){
          const tok=new Token("R");
          let integer = (t.value==="R*");
          if (!integer){ const pk=S.peekTok(); if (pk && pk.type==="OP" && pk.value==="*") { S.nextTok(); integer=true; } }
          tok.integer=integer;

          const nameTok=S.nextTokReq(integer?"R* expects a variable name":"R expects a variable name");
          if (nameTok.type!=="ID" || !/^[A-Za-z_][A-Za-z0-9_]*$/.test(nameTok.value)){
            throw ErrorReporter.fromToken(integer?"R* expects a variable name":"R expects a variable name", nameTok);
          }
          tok.var_name=nameTok.value;

          this.expectOp(S,"(", "R");
          const lowTok = S.nextTokReq("R lower bound");
          const highTok= S.nextTokReq("R upper bound");
          if (lowTok.type!=="NUM" || highTok.type!=="NUM") {
            throw ErrorReporter.fromToken("R bounds must be numeric and space-delimited", lowTok.type!=="NUM"?lowTok:highTok);
          }
          tok.addArgument(parseFloat(lowTok.value));
          tok.addArgument(parseFloat(highTok.value));
          this.expectOp(S,")","R");

          const low  = parseFloat(lowTok.value);
          const high = parseFloat(highTok.value);
         //add variable
        addVariable(tok.var_name,low,high,integer);

          rule.addToken(tok, sec); continue;
        }

        // I
        if (t.type==="ID" && t.value==="I"){
          this.expectOp(S,"(","I");
          const typTok=S.nextTokReq("I instance type");
          const allowed=new Set(["Cube","Sphere","Cylinder","CubeX","CubeY"]);
          if (typTok.type!=="ID" || !allowed.has(typTok.value)) throw ErrorReporter.fromToken("I unknown instance type", typTok);
          const tok=new Token("I"); tok.addInstanceType(typTok.value);

          let pk=S.peekTok();
          if (pk && !(pk.type==="OP" && pk.value===")")){
            const a1=S.nextTokReq("I first argument");
            if (a1.type==="NUM") tok.addArgument(parseFloat(a1.value));
            else if (a1.type==="ID") { tok.var_names[0]=a1.value; tok.addArgument(0); }
            else throw ErrorReporter.fromToken("I: first argument must be number or identifier", a1);

            pk=S.peekTok();
            if (pk && !(pk.type==="OP" && pk.value===")")){
              const a2=S.nextTokReq("I second argument");
              if (a2.type!=="NUM") throw ErrorReporter.fromToken("I: second argument must be numeric (space-delimited)", a2);
              tok.addArgument(parseFloat(a2.value));

              pk=S.peekTok();
              if (pk && !(pk.type==="OP" && pk.value===")")){
                const a3=S.nextTokReq("I third argument");
                if (a3.type!=="NUM") throw ErrorReporter.fromToken("I: third argument must be numeric (space-delimited)", a3);
                tok.addArgument(parseFloat(a3.value));
              }
            }
          }
          this.expectOp(S,")","I");
          rule.addToken(tok, sec); continue;
        }

        // A
        if (t.type==="ID" && t.value==="A"){
          this.expectOp(S,"(","A");
          const tok=new Token("A");
          for (let i=0;i<2;i++){
            const a=S.nextTokReq(`A missing argument ${i}`);
            if (a.type==="NUM") tok.addArgument(parseFloat(a.value));
            else if (a.type==="ID") { tok.var_names[i]=a.value; tok.addArgument(0); }
            else if (a.type==="MATH"){tok.var_names[i]=a.value; tok.addArgument(0);}
            else throw ErrorReporter.fromToken(`A: argument ${i} must be number or identifier ${a.type}  ${a.value} `, a);
          }
          this.expectOp(S,")","A");
          rule.addToken(tok, sec); continue;
        }

        if (t.type==="OP" && (t.value==="[" || t.value==="]")) { rule.addToken(new Token(t.value), sec); continue; }
        if (t.type==="OP" && (t.value==="{" || t.value==="}" || t.value==="*")) { continue; }
        if (t.type==="ID") { rule.addToken(new Token(t.value), sec); continue; }

        throw ErrorReporter.fromToken("Unknown/unsupported token in rule", t);
      }
    } catch(e){
      errorout(e); if (e instanceof ParseError) throw e;
      throw new ParseError(`Tokenization/Read error: ${e.message||e}`, 0,0,"");
    }
  }

  /* --- only resolve when we can evaluate to a finite number; otherwise keep var_names --- */
  update_token(tok){
    const tryEval = (expr) => {
      const val = MathS2_js(expr);
      return Number.isFinite(val) ? val : null;
    };
    const maybeFillIdx = (i) => {
      if (tok.var_names[i] !== "") {
        const n = tryEval(tok.var_names[i]);
        if (n !== null) { tok.arguments[i] = n; tok.var_names[i] = ""; }
      }
    

    if (["D","S","T"].includes(tok.token_name)) {
      maybeFillIdx(0); maybeFillIdx(1); maybeFillIdx(2);
    } else if (tok.token_name === "A") {
      maybeFillIdx(0);
    } else if (tok.token_name === "I") {
      maybeFillIdx(0);
    }
 };
}
// unify MathS to match MathS2_js behavior
  MathS(input){ return String(MathS2_js(input)); }

/* ======================================================= */

//                                        Recurse Rules

/* ======================================================= */

  Recurse(rule){
    const out=[];
    for (let i=0;i<rule.var_counter;i++){ addVariableInstance(rule.var_names[i]); }

    if (rule.var_name!==""){ const v=parseFloat(this.MathS(rule.var_name));
      if (!Number.isFinite(v)) throw new RuntimeGrammarError(`Repeat not finite for rule ${rule.rule_name}`); rule.repeat=v; }
    if (!Number.isFinite(rule.repeat) || rule.repeat<0) throw new RuntimeGrammarError(`Bad repeat count for ${rule.rule_name}`);

    // Correct probability handling: choose main with probability p, else alternate
    if (rule.alternate){
      const roll = RNG.random();
      if (roll >= rule.probability) {
        const more = this.Recurse(rule.alternate);
        for (const t of more){ this.update_token(t); out.push(t); }
        return out;
      }
    }

    const runSection = (tokens) => {
      for (const tt of tokens){
        if (tt.isRule()!==""){
          const idx=this.findRule(tt.token_name);
          if (idx!==-1){
            const more=this.Recurse(this.rule_list[idx]);
            for (const t of more){ this.update_token(t); out.push(t); }
          } else throw new RuntimeGrammarError(`Unknown rule referenced: ${tt.token_name}`);
        } else {
          const cc=Token.fromToken(tt); this.update_token(cc); out.push(cc);
        }
      }
    };

    for (let sec=0; sec<3; sec++){
      if (sec===1){
        const varCount=new Variable(rule.rule_name+"_count",0,0,true); variable_list.push(varCount);
        for (let l=0;l<rule.repeat;l++){ varCount.value=l; runSection(rule.section_tokens[sec]); }
        removeVariable(rule.rule_name+"_count");
      } else runSection(rule.section_tokens[sec]);
    }

   // for (let i=0;i<rule.var_counter;i++) removeVariable(rule.var_names[i]);
    return out;
  }

  findRule(name){ for (let i=0;i<this.rule_list.length;i++) if (this.rule_list[i].rule_name===name) return i; return -1; }

  makeStream(header){ const L=new GrammarLexer(header); return this.makeStreamFromLexer(L); }
  makeStreamFromLexer(L){
    let i=0; const toks=L.tokens;
    const skip=()=>{ while (i<toks.length && (toks[i].type==="WS"||toks[i].type==="CMT")) i++; };
    const peekTok = ()=>{ let j=i; while (j<toks.length && (toks[j].type==="WS"||toks[j].type==="CMT")) j++; return j<toks.length? toks[j]: null; };
    const nextTok = ()=>{ skip(); return i<toks.length? toks[i++]: null; };
    const nextTokReq = (msg)=>{ const t=nextTok(); if (!t) throw new ParseError(msg); return t; };
    const nextVal = ()=>{ skip(); return i<toks.length? toks[i++].value: null; };
    return { peekTok, nextTok, nextTokReq, nextVal };
  }
  expectOp(S, op, ctx){
    const t=S.nextTok(); if (!t || t.type!=="OP" || t.value!==op) {
      throw t ? ErrorReporter.fromToken(`${ctx} expects '${op}'`, t) : new ParseError(`${ctx} expects '${op}'`);
    }
  }
  isNumStr(s){ return typeof s==="string" && /^[-+]?(?:\d+(?:\.\d+)?|\.\d+)(?:[eE][-+]?\d+)?$/.test(s); }
}



/* EXECUTION PIPE: prepare() ===================…Code *

/*  EXECUTION PIPE: prepare() ====================== */
function prepare(tokens,scene) {
  const ctx = new Context();
  
  const deps = { addVariable }; // MathS2_js is called directly inside performAction
  for (const t of tokens) t.performAction(ctx, scene, deps);
  return { ctx, scene };
}



/* Pretty printers ====================== */ func…Code *

/* Pretty printers ====================== */
function formatMat4(m){
  const r = (a,b,c,d)=>`${a.toFixed(3)} ${b.toFixed(3)} ${c.toFixed(3)} ${d.toFixed(3)}`;
  return [
    r(m[0], m[4], m[8],  m[12]),
    r(m[1], m[5], m[9],  m[13]),
    r(m[2], m[6], m[10], m[14]),
    r(m[3], m[7], m[11], m[15]),
  ].join("\n");
}
function printScene(scene){
  const items = scene.getAll();
  if (!items.length) return "(scene empty)";
  let out = `Scene items: ${items.length}\n`;
  for (let i=0;i<items.length;i++){
    const it = items[i];
    out += `\n#${i} arg=${it.arg} val=${it.val}\n${formatMat4(it.transform)}\n`;
  }
  return out;
}



/* = Integrated Editor + Renderer + Tutorial (wit…Code *

/* = Integrated Editor + Renderer + Tutorial (with DOM console) ================== */
(function () {
  'use strict';

  /* ---------- Safe DOM getters ---------- */
  const need = (id) => {
    const el = document.getElementById(id);
    if (!el){ console.warn("[UI] Missing element #${id}");return;}
   return el;
  };

  /* ---------- Grab UI ---------- */
  const src           = need('src');
  const hl            = need('hl');
  const codewrap      = need('codewrap');
  const wrapBtn       = need('wrapBtn');

  const out           = need('out');           // console panel (DOM rendered)
  const goBtn         = need('go');
  const clrBtn        = need('clr');
  const statusEl      = need('status');
  const orbitToggle   = need('orbitToggle');

  const refDock       = need('refDock');
  const refHead       = need('refHead');
  const refToggleLabel= need('refToggleLabel');

  /* ---------- Console helpers (DOM-rendered) ---------- */
  function setStatus(msg){ if (statusEl) statusEl.textContent = msg; }

  function renderConsoleLine(kind, text) {
    if (!out){console.error("out not found.exiting."); return;}
    const line = document.createElement('div');
    line.className = `console-line console-${kind}`;

    const time = document.createElement('span');
    time.className = 'console-time';
    time.textContent = `[${new Date().toLocaleTimeString()}]`;

    const msg = document.createElement('span');
    msg.className = 'console-msg';
    msg.textContent = String(text);

    line.appendChild(time);
    line.appendChild(msg);
    out.appendChild(line);
    out.scrollTop = out.scrollHeight;
  }

  const info  = (t)=> renderConsoleLine('info',  t);
  const warn  = (t)=> renderConsoleLine('warn',  t);
  const error = (t)=> renderConsoleLine('error', t);

  /* ---------- Scene & Renderer ---------- */
  let scene, renderer;
  
    if (typeof Scene !== 'function' || typeof WebGLSceneRenderer !== 'function') {
      error('[Renderer] Scene or WebGLSceneRenderer is not loaded globally.');
      return;
    }
    try {
      scene = new Scene();
      renderer = new WebGLSceneRenderer('#glcanvas', { wireframe: true });
      renderer.setScene(scene);
      info('Renderer initialised.');
    } catch (err) {
      error("[Renderer] Failed to initialize.");
    }
 

  /* ---------- Output header helper (kept) ---------- */
  const TUTORIAL_NAME = 'Visual Grammar 101';
  function updateOutHeader(stepIndex, total, title, done=false){
    // Keep the first line of console reflecting tutorial status as an info line
    renderConsoleLine('info',
      `Tutorial: ${TUTORIAL_NAME} — Step ${Math.min(stepIndex,total)}/${total}${title ? ` (${title})` : ''}${done ? ' • ✅ Completed' : ''}`
    );
  }

  /* ---------- Word Wrap toggle ---------- */
  let wrapOn = false;
  function setWrap(on){
    wrapOn = !!on;
    codewrap && codewrap.classList.toggle('wrap-on', wrapOn);
    if (wrapBtn) wrapBtn.textContent = wrapOn ? '⤶ Wrap: On' : '⤶ Wrap: Off';
    updateHighlight();
  }
  wrapBtn && wrapBtn.addEventListener('click', ()=> setWrap(!wrapOn));

  /* ---------- Highlighter ---------- */
  // simple hue by name
  function nameHue(name){ let h=0; for (let i=0;i<name.length;i++) h=(h*31 + name.charCodeAt(i))>>>0; return h%360; }
  const ctxStyle = (name, a=0.18) => `style="background:hsla(${nameHue(name)},70%,20%,${a});"`;

  const TOK_RE = /(\bR\*?\b|\bA\b|\bS\b|\bT\b|\bD\b|\bI\b|->|\[|\]|\{|\}|\||\(|\)|-?\d+(?:\.\d+)?|[A-Za-z_]\w*|\s+|.)/g;
  const esc = (s)=> s.replace(/[&<>"]/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));

  function collectSymbols(text){
    const varNames = new Set();
    const ruleNames = new Set();
    const reR = /(^|\s)R\*?\s+([A-Za-z_]\w*)/g;
    let m; while ((m = reR.exec(text))) varNames.add(m[2]);
    const reRule = /^([A-Za-z_]\w*)\s.*?->/gm;
    while ((m = reRule.exec(text))) ruleNames.add(m[1]);
    return { varNames, ruleNames };
  }

  function highlightToHTML(text){
    const { varNames, ruleNames } = collectSymbols(text || '');
    let html = '', m;
    while ((m = TOK_RE.exec(text || ''))){
      const tok = m[0];
      if (/^\s+$/.test(tok)) { html += tok.replace(/ /g,'&nbsp;').replace(/\n/g,'<br>'); continue; }
      if (/^(R\*?|A|S|T|D|I)$/.test(tok))      html += `<span class="tok-kw">${tok}</span>`;
      else if (tok === '->' || tok === '|')    html += `<span class="tok-op">${tok}</span>`;
      else if (tok === '(' || tok === ')')     html += `<span class="tok-paren">${tok}</span>`;
      else if (tok === '[' || tok === ']' || tok === '{' || tok === '}')
                                               html += `<span class="tok-brack">${tok}</span>`;
      else if (/^-?\d+(?:\.\d+)?$/.test(tok))  html += `<span class="tok-num">${tok}</span>`;
      else if (/^[A-Za-z_]\w*$/.test(tok)){
        if (varNames.has(tok))      html += `<span class="ctx tok-var" ${ctxStyle(tok,0.18)}>${tok}</span>`;
        else if (ruleNames.has(tok))html += `<span class="ctx tok-rule" ${ctxStyle(tok,0.14)}>${tok}</span>`;
        else                        html += esc(tok);
      } else html += esc(tok);
    }
    return html;
  }

  let hlRAF = null;
  function updateHighlight(){
    if (!hl || !src) return;
    if (hlRAF) cancelAnimationFrame(hlRAF);
    hlRAF = requestAnimationFrame(()=> {
      hl.innerHTML = highlightToHTML(src.value || '');
      hlRAF = null;
    });
  }
  src && src.addEventListener('input', updateHighlight);
  src && src.addEventListener('scroll', ()=> { if (hl && src){ hl.scrollTop = src.scrollTop; hl.scrollLeft = src.scrollLeft; } });

  /* ---------- Collapsible Mini‑Reference ---------- */
  refHead && refHead.addEventListener('click', () => {
    if (!refDock) return;
    refDock.classList.toggle('min');
    const open = !refDock.classList.contains('min');
    if (refToggleLabel) refToggleLabel.textContent = open ? 'Hide ▲' : 'Show ▼';
  });

  /* ---------- Grammar pipeline with logging ---------- */
  function runGrammarPipeline(text) {
    if (!scene || !renderer) {
      setStatus('Renderer not ready.');
      warn('[Pipeline] Renderer not ready.');
      return;
    }
    if (!text) {
      setStatus('Nothing to run.');
      warn('Nothing to run — the editor is empty.');
      return;
    }
    try {
      scene.clear?.();
      if (typeof Grammar !== 'function' || typeof prepare !== 'function') {
        throw new Error('Grammar/prepare not available globally.');
      }
      const g = new Grammar(text);
      info(`Tokenising… (${g.tokens_new?.length ?? 'unknown'} tokens)`);
      prepare(g.tokens_new, scene);
      renderer.setScene?.(scene);
      renderer.invalidate?.();
      setStatus('Done.');
      info(`Build complete. Scene items: ${scene.getAll?.().length ?? 'n/a'}`);
    } catch (e) {
      setStatus('Error.');
      error(`Grammar error: ${e?.message || e}`);
      // If you can detect error ranges, call flashErrorRange(start, end)
    }
  }

  /* ---------- Actions: Run / Clear (logs + status) ---------- */
  goBtn && goBtn.addEventListener('click', () => {
    if (!src) return;
    const text = (src.value || '').trim();
    if (!text){
      setStatus('Nothing to run.');
      warn('Nothing to run — the editor is empty.');
      return;
    }
    setStatus('Running…');
    info('Parsing grammar and preparing scene…');
    runGrammarPipeline(text);
  });

  clrBtn && clrBtn.addEventListener('click', () => {
    if (!src) return;
    src.value = '';
    updateHighlight();
    setStatus('Cleared.');
    info('Editor cleared.');
    scene && scene.clear?.();
    renderer && renderer.setScene?.(scene);
    src.focus();
  });

  /* ---------- Very slow auto‑orbit (play/pause) ---------- */
  let orbitOn = false, orbitHandle = null;
  function orbitLoop(){
    if (!orbitOn || !renderer) return;
    try {
      renderer.theta = (renderer.theta || 0) + 0.0025;
      renderer.invalidate?.();
    } catch(e){ warn('[Orbit] invalidate failed: ' + (e?.message || e)); }
    orbitHandle = requestAnimationFrame(orbitLoop);
  }
  orbitToggle && orbitToggle.addEventListener('click', ()=>{
    orbitOn = !orbitOn;
    orbitToggle.textContent = orbitOn ? '⏸ Auto Orbit' : '▶ Auto Orbit';
    if (orbitOn){ orbitLoop(); } else { if (orbitHandle) cancelAnimationFrame(orbitHandle); orbitHandle=null; }
  });

  /* ---------- Tutorial (kept lightweight; logs into console header) ---------- */
  const P1 = `X -> R len ( 0.15 0.3 ) `;
  const P2 = `R radius ( 0.75 1.75 ) `;
  const P3 = `R* count ( 21 63 ) `;
  const P4 = `A ( 90 0 ) `;
  const P5 = `S ( 1 1 10 ) [ Y ]\n\n`;
  const P6 = `Y 19 len count radius ->  A ( 19 2 )  [ YB ]\n`;
  const P7 = `YB -> T ( -radius 0 0 ) | [ YB1 ] [ YB2 ]\n\n`;
  const P8 = `YB1 ->  S ( radius len 1 ) D ( 1 0.03 1 )  I ( CubeX 2 0 1 )\n`;
  const P9 = `YB2 ->  A ( 180 0 )  S ( radius len 1 ) D ( 1 0.03 1 )  I ( CubeX 2 0 1 )`;
  const DEMO = [P1,P2,P3,P4,P5,P6,P7,P8,P9];

  function initTutorial(){
    // If overlay not present, preload editor and log
    const overlay = document.getElementById('tut');
    if (!overlay){
      info('[Tutorial] Overlay UI not present; loading demo grammar into editor.');
      if (src){ src.value = DEMO.join(''); updateHighlight(); }
      updateOutHeader(0, DEMO.length, 'Simple Mode');
      return;
    }
    // Otherwise your existing tutorial engine can run here.
    updateOutHeader(0, DEMO.length, 'Ready');
}
/* ---------- Tutorial Start Button ---------- */
function addTutorialBtn() {
  const bar = document.getElementById("toolbar") || document.body;
  if (!bar) return;
  if (document.getElementById("tutorialBtn")) return; // prevent dupes

  const btn = document.createElement("button");
  btn.id = "tutorialBtn";
  btn.className = "btn secondary";
  btn.textContent = "▶ Start Tutorial";
 
 btn.addEventListener("click", () => {
  initTutorial();
  bar.appendChild(btn);
  });


}

addTutorialBtn();

})();



/* Smart Editor v0.1 (non-destructive add-on) …Code *

/*    Smart Editor v0.1 (non-destructive add-on)
   - Requires: #src (textarea), #hl (pre.highlighter), #codewrap (container)
   - Works alongside your current scheduleHighlight() — this module
     re-renders #hl with data-attributes so tooltips/autocomplete work.
====================================================== */


(() => {
  'use strict';
  console.info("[SmartEditor] Initialising…");

  // ---- safe UI grab ------------------------------------------------
  const need = (id) => document.getElementById(id);
  const ui = {
    src: need('src'),
    hl: need('hl'),
    wrap: need('codewrap')
  };
  if (!ui.src || !ui.hl || !ui.wrap) {
    console.error('[SmartEditor] Missing required elements (#src/#hl/#codewrap). Aborting.');
    return;
  }
function renderHL() {
  const text = ui.src.value || "";
  const tokens = tokenize(text);
  const { vars, rules } = collectSymbols(text);
  const { errIdx } = diagnostics(tokens);

  let html = "";
  for (const t of tokens) {
    let cls = `tok-${t.type}`;
    if (errIdx.has(t.i)) cls += " tok-error";
    html += `<span class="${cls}">${t.raw
      .replace(/&/g,"&amp;")
      .replace(/</g,"&lt;")
      .replace(/>/g,"&gt;")}</span>`;
  }
  ui.hl.innerHTML = html;
}


  // ---- caret pixel locator (for menus/tooltips) --------------------
  function caretXY(textarea) {
    const i = textarea.selectionStart;
    const div = document.createElement('div');
    const cs = getComputedStyle(textarea);
    div.style.cssText = [
      'position:absolute','white-space:pre-wrap','word-wrap:break-word','visibility:hidden',
      `font:${cs.font}`,`padding:${cs.padding}`,
      `border:${cs.borderWidth} ${cs.borderStyle} ${cs.borderColor}`,
      `width:${textarea.clientWidth}px`
    ].join(';');
    const before = textarea.value.slice(0,i);
    const after  = textarea.value.slice(i);
    div.textContent = before;
    const span = document.createElement('span'); 
    span.textContent = '\u200b';
    div.appendChild(span);
    div.appendChild(document.createTextNode(after));
    document.body.appendChild(div);
    const rDiv = div.getBoundingClientRect();
    const rSpan = span.getBoundingClientRect();
    const rTA = textarea.getBoundingClientRect();
    const x = rTA.left + (rSpan.left - rDiv.left);
    const y = rTA.top + (rSpan.top - rDiv.top) + window.scrollY;
    document.body.removeChild(div);
    return {x,y};
  }

  // ---- tokenizer ---------------------------------------------------
  const TOK_RE = /(\bR\*?\b|\bA\b|\bS\b|\bT\b|\bD\b|\bI\b|->|\[|\]|\{|\}|\||\(|\)|-?\d+(?:\.\d+)?|[A-Za-z_]\w*|\s+|.)/g;
  function tokenize(text) {
    TOK_RE.lastIndex = 0;
    const tokens = [];
    let m, idx = 0, pos = 0;
    while ((m = TOK_RE.exec(text))) {
      const raw = m[0];
      let type = 'sym';
      if (/^\s+$/.test(raw)) type='ws';
      else if (/^(R\*?|A|S|T|D|I)$/.test(raw)) type='kw';
      else if (raw==='->' || raw==='|') type='op';
      else if (raw==='('||raw===')') type='par';
      else if (raw==='['||raw===']'||raw==='{'||raw==='}') type='br';
      else if (/^-?\d+(?:\.\d+)?$/.test(raw)) type='num';
      else if (/^[A-Za-z_]\w*$/.test(raw)) type='id';
      tokens.push({i: idx++, type, raw, start: pos, end: pos+raw.length});
      pos += raw.length;
    }
    return tokens;
  }

  // ---- symbol collection for context (vars/rules) ------------------
  function collectSymbols(text) {
    const vars = new Set(), rules = new Set();
    const reR = /(^|\s)R\*?\s+([A-Za-z_]\w*)/g; let m;
    while ((m = reR.exec(text))) vars.add(m[2]);
    const reRule = /(^|\n)\s*([A-Za-z_]\w*)\s.*?->/g;
    while ((m = reRule.exec(text))) rules.add(m[2]);
    return {vars, rules};
  }

  // ---- diagnostics -------------------------------------------------
  function diagnostics(tokens) {
    const errIdx = new Set();
    const stack = [];
    tokens.forEach(t => {
      if (t.type==='br' || t.type==='par') {
        if (t.raw==='['||t.raw==='('||t.raw==='{') stack.push(t);
        else {
          const open = stack.pop();
          const ok =
            (open && open.raw==='[' && t.raw===']') ||
            (open && open.raw==='(' && t.raw===')') ||
            (open && open.raw==='{' && t.raw==='}');
          if (!ok) {
            errIdx.add(t.i);
            if (open) errIdx.add(open.i);
            console.warn("[SmartEditor] Mismatched bracket/paren near token", t);
          }
        }
      }
    });
    stack.forEach(t => {
      errIdx.add(t.i);
      console.warn("[SmartEditor] Unclosed bracket/paren near token", t);
    });
    return {errIdx};
  }

  // ---- (rest of the code unchanged from your version)… -------------
  // buildMathHints, renderHL, syncScroll, autocomplete, tooltip,
  // key bindings, etc. all follow as in your draft.

  // ---- initial render ----------------------------------------------
  renderHL();
  console.info("[SmartEditor] Ready.");

})();



/* Grammar Fragments */ (function(){ const $ =…Code *

/*  Grammar Fragments */
(function(){
  const $ = (id) => document.getElementById(id);
  const src = $('src');
  const out = $('out');
  const statusEl = $('status');
  const nameInput = $('nameId');
  const saveBtn = $('saveBtn');

// refs
const chooser = document.getElementById('nameChooser');

// build dropdown from localStorage 'grammar_saves'
function refreshChooser(selectedId){
  if(!chooser) return;
  const saves = loadSaves();
  const ids = Object.keys(saves).sort((a,b)=>{
    const da = Date.parse(saves[a]?.saved_at||0) || 0;
    const db = Date.parse(saves[b]?.saved_at||0) || 0;
    return db - da || a.localeCompare(b);
  });

  chooser.innerHTML = '';
  const opt0 = document.createElement('option');
  opt0.value = '';
  opt0.textContent = ids.length ? 'Choose saved…' : 'No saved entries';
  opt0.disabled = true; opt0.selected = true;
  chooser.appendChild(opt0);

  ids.forEach(id=>{
    const o = document.createElement('option');
    o.value = id; o.textContent = id;
    if (selectedId && id === selectedId) o.selected = true;
    chooser.appendChild(o);
  });
}

// load on selection
chooser?.addEventListener('change', ()=>{
  const id = chooser.value;
  const saves = loadSaves();
  const entry = saves[id];
  if(!entry){ log(`No entry for "${id}".`, 'warn'); return; }
  if (nameInput) nameInput.value = id;
  if (src){
    src.value = entry.input_grammar || '';
    // let your highlighter update if it listens to 'input'
    src.dispatchEvent(new Event('input', {bubbles:true}));
  }
  statusEl && (statusEl.textContent = 'Loaded.');
  log(`Loaded "${id}" (${(entry.input_grammar||'').length} chars).`, 'info');
});

// call once on init and after every save
refreshChooser();

  function log(msg, type='info'){
    if(!out) return;
    const div = document.createElement('div');
    div.className = `console-line ${type}`;
    div.textContent = msg;
    out.appendChild(div);
    out.scrollTop = out.scrollHeight;
  }

  function loadSaves(){
    try{
      const raw = localStorage.getItem('grammar_saves');
      if(!raw) return {};
      const parsed = JSON.parse(raw);
      return (parsed && typeof parsed === 'object') ? parsed : {};
    }catch(e){
      console.warn('Failed to parse grammar_saves:', e);
      return {};
    }
  }
  function persistSaves(obj){
    try{ localStorage.setItem('grammar_saves', JSON.stringify(obj, null, 2)); }
    catch(e){ console.warn('Failed to persist grammar_saves:', e); }
  }



  function sanitizeName(s){
    return String(s||'').toLowerCase().trim()
      .replace(/[^a-z0-9-_]+/g, '-')
      .replace(/^-+|-+$/g, '')
      .slice(0, 80);
  }
  function genRandomId(){
    return 'id_' + Math.random().toString(36).slice(2,7) + Math.floor(Math.random()*1e6).toString(36);
  }
  function timestampCompact(){
    const d=new Date();
    const p=n=>String(n).padStart(2,'0');
    return `${d.getFullYear()}${p(d.getMonth()+1)}${p(d.getDate())}-${p(d.getHours())}${p(d.getMinutes())}${p(d.getSeconds())}`;
  }
  function downloadJSON(filename, text){
    const blob = new Blob([text], {type:'application/json'});
    const a = document.createElement('a');
    a.href = URL.createObjectURL(blob);
    a.download = filename;
    document.body.appendChild(a);
    a.click();
    a.remove();
    setTimeout(()=>URL.revokeObjectURL(a.href), 1500);
  }

  saveBtn?.addEventListener('click', ()=>{
    const saves = loadSaves();

    const userName = sanitizeName(nameInput?.value);
    const base = userName || genRandomId();
    let id = base;

    if (Object.prototype.hasOwnProperty.call(saves, id)) {
      id = `${base}-${timestampCompact()}`;
    }

    const entry = {
      unique_name_identifier: id,
      saved_at: new Date().toISOString(),
      input_grammar: String(src?.value ?? '')
    };

    saves[id] = entry;

    const jsonText = JSON.stringify(saves, null, 2);
    persistSaves(saves);
// inside your existing save handler, after persistSaves(saves):
 refreshChooser(id);
    downloadJSON('grammar_saves.json', jsonText);

    if (nameInput) namechooser.value = id;
    if (statusEl) statusEl.textContent = 'Saved.';
    log(`Saved "${id}" (${entry.input_grammar.length} chars).`, 'info');
  });
})();



