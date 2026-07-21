#include "WebAppServer.h"

#include <mbedtls/base64.h>
#include <mbedtls/sha1.h>
#include <mbedtls/version.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "RadioProtocol.h"

namespace {

constexpr uint32_t kHttpReadTimeoutMs = 80;
constexpr uint32_t kWsHandshakeTimeoutMs = 120;
constexpr size_t kHttpRequestMax = 1024;
constexpr size_t kWsMessageMax = 320;

const char kWebSocketGuid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

const char kIndexHtml[] =
    R"APPHTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no,viewport-fit=cover">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="theme-color" content="#101318">
<link rel="manifest" href="/manifest.json">
<title>RadioMaster Phone RC</title>
<style>
:root{
  color-scheme:dark;
  --bg:#101318;
  --panel:#181d24;
  --panel2:#202733;
  --line:#323b48;
  --text:#eef3f8;
  --muted:#8f9aa8;
  --green:#50d47a;
  --red:#ff5a66;
  --amber:#f5bd4f;
  --blue:#5fb3ff;
  --cyan:#5ee0d8;
  --purple:#b58cff;
}
.routeSeg button{min-width:54px}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent;-webkit-touch-callout:none;-webkit-user-select:none;user-select:none}
html,body{margin:0;width:100%;height:100%;overflow:hidden;background:var(--bg);color:var(--text);font-family:Inter,system-ui,-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif}
body{touch-action:none}
.app{width:100vw;height:100svh;display:grid;grid-template-rows:auto auto minmax(0,1fr) auto auto;gap:5px;padding:6px max(6px,env(safe-area-inset-right)) max(6px,env(safe-area-inset-bottom)) max(6px,env(safe-area-inset-left))}
.top{display:grid;grid-template-columns:minmax(0,1fr) auto auto auto;gap:5px;align-items:center}
.title{min-width:0;overflow:hidden;text-overflow:ellipsis;font-size:12px;font-weight:800;letter-spacing:0;text-transform:uppercase;white-space:nowrap}
.chips{display:flex;gap:4px;align-items:center;justify-content:flex-end;min-width:0}
.chip{height:22px;padding:0 6px;border:1px solid var(--line);border-radius:6px;background:#151a21;color:var(--muted);display:flex;align-items:center;gap:4px;font-size:10px;font-weight:700;white-space:nowrap}
.dot{width:6px;height:6px;border-radius:50%;background:var(--red);box-shadow:0 0 0 2px rgba(255,90,102,.1)}
.chip.ok .dot{background:var(--green);box-shadow:0 0 0 3px rgba(80,212,122,.12)}
.seg{display:flex;gap:2px;padding:2px;border:1px solid var(--line);border-radius:6px;background:#12161c}
button{font:inherit;color:inherit;border:0;background:none;touch-action:manipulation;-webkit-touch-callout:none;-webkit-user-select:none;user-select:none}
.seg button{height:20px;min-width:40px;border-radius:5px;color:var(--muted);font-size:10px;font-weight:800}
.seg button.active{background:var(--panel2);color:var(--text)}
.switches{display:grid;grid-template-columns:repeat(10,minmax(0,1fr));gap:4px}
.sw{min-width:0;height:30px;padding:0 1px;border:1px solid var(--line);border-radius:6px;background:var(--panel);display:flex;flex-direction:column;align-items:center;justify-content:center;gap:0;line-height:1}
.sw b{font-size:10px;letter-spacing:0}
.sw span{font-size:7px;color:var(--muted);font-weight:800;letter-spacing:0}
.sw.on{border-color:rgba(80,212,122,.65);background:linear-gradient(180deg,#1f3a2a,#18251d);color:#dcffe7}
.sw.warn.on{border-color:rgba(255,90,102,.75);background:linear-gradient(180deg,#412127,#25181b);color:#ffe1e5}
.sw.moment.on{border-color:rgba(245,189,79,.75);background:linear-gradient(180deg,#423421,#251f17);color:#fff1cf}
.deck{min-height:0;display:grid;grid-template-columns:minmax(130px,1fr) minmax(118px,.72fr) minmax(130px,1fr);gap:5px;align-items:stretch}
.stickPanel,.centerPanel,.monitor{border:1px solid var(--line);border-radius:7px;background:var(--panel);box-shadow:0 12px 36px rgba(0,0,0,.18)}
.stickPanel{position:relative;display:grid;place-items:center;overflow:hidden;min-height:160px}
.stickTitle{position:absolute;left:8px;top:6px;font-size:10px;font-weight:800;color:var(--muted);letter-spacing:.03em}
.axisLabel{position:absolute;font-size:8px;font-weight:800;color:#657181;letter-spacing:.03em}
.axisLabel.top{top:28px}.axisLabel.bottom{bottom:8px}.axisLabel.left{left:8px}.axisLabel.right{right:8px}
.gimbal{width:min(38svh,32vw,205px);height:min(38svh,32vw,205px);min-width:116px;min-height:116px;border-radius:50%;position:relative;background:radial-gradient(circle at 50% 52%,#273140 0 33%,#1d232d 34% 62%,#141920 63% 100%);border:1px solid #394453;box-shadow:inset 0 0 0 1px rgba(255,255,255,.03),inset 0 22px 42px rgba(255,255,255,.03)}
.gimbal:before,.gimbal:after{content:"";position:absolute;background:#394453;opacity:.9}
.gimbal:before{left:13%;right:13%;top:50%;height:1px}.gimbal:after{top:13%;bottom:13%;left:50%;width:1px}
.ring{position:absolute;inset:17%;border:1px dashed #485566;border-radius:50%}
.knob{position:absolute;left:50%;top:50%;width:29%;height:29%;border-radius:50%;transform:translate(-50%,-50%);background:radial-gradient(circle at 35% 25%,#f8fbff 0,#b8c6d8 19%,#687585 55%,#3c4654 100%);border:1px solid #95a4b6;box-shadow:0 12px 24px rgba(0,0,0,.35)}
.centerPanel{display:grid;grid-template-rows:auto 1fr auto;gap:5px;padding:6px;min-width:0}
.telemetry{display:grid;grid-template-columns:1fr 1fr;gap:4px}
.meter{border:1px solid var(--line);border-radius:6px;background:#12161c;padding:5px 4px}
.meter span{display:block;color:var(--muted);font-size:7px;font-weight:800;letter-spacing:0}
.meter b{display:block;margin-top:1px;font-size:12px}
.deadman{min-height:44px;border-radius:6px;border:1px solid #485566;background:#171d25;color:var(--text);font-size:11px;font-weight:900;letter-spacing:0}
.deadman.on{background:linear-gradient(180deg,#164d2b,#15291f);border-color:rgba(80,212,122,.8);box-shadow:0 0 0 3px rgba(80,212,122,.12)}
.trimGrid{display:grid;grid-template-columns:1fr 1fr;gap:4px}
.miniBtn{height:28px;border:1px solid var(--line);border-radius:6px;background:#151a21;color:var(--muted);font-size:10px;font-weight:800}
.miniBtn:active{background:#242c38;color:var(--text)}
.controlStrip{display:grid;grid-template-columns:minmax(110px,1.5fr) 54px minmax(100px,1fr) 54px;gap:4px;align-items:center;min-width:0}
.servoControl{height:30px;display:grid;grid-template-columns:auto minmax(50px,1fr) 28px;gap:5px;align-items:center;color:var(--muted);font-size:8px;font-weight:800;min-width:0}
.servoControl input{width:100%;min-width:0;accent-color:var(--cyan)}
.servoControl b{color:var(--text);font-size:10px;text-align:right}
.modeBtn,.taskBtn,.taskSelect{height:30px;border:1px solid var(--line);border-radius:6px;background:var(--panel);font-size:9px;font-weight:800;min-width:0}
.modeBtn.on{border-color:rgba(80,212,122,.65);background:#1f3a2a;color:#dcffe7}
.taskSelect{padding:0 5px;color:var(--text);background:#151a21}
.taskBtn{color:var(--amber)}
.taskBtn.on{border-color:rgba(245,189,79,.75);background:#423421;color:#fff1cf}
.monitor{padding:5px;display:grid;grid-template-columns:repeat(8,minmax(0,1fr));gap:4px;min-height:52px}
.bar{min-width:0}
.bar label{display:flex;justify-content:space-between;gap:2px;font-size:7px;font-weight:800;color:var(--muted);line-height:1.1}
.track{height:6px;margin-top:2px;border-radius:999px;background:#11161c;border:1px solid #2a3340;overflow:hidden}
.fill{height:100%;width:50%;background:linear-gradient(90deg,var(--blue),var(--green));border-radius:999px}
.danger{color:var(--red)!important}.good{color:var(--green)!important}.warnText{color:var(--amber)!important}
@media (orientation:portrait){
  .top{grid-template-columns:minmax(0,1fr) auto auto;grid-template-areas:"title route seg" "chips chips chips";align-items:start}
  .title{grid-area:title}
  .chips{grid-area:chips;justify-content:space-between}
  .routeSeg{grid-area:route}
  .protoSeg{grid-area:seg}
  .switches{grid-template-columns:repeat(5,minmax(0,1fr))}
  .deck{grid-template-columns:1fr 1fr;grid-template-rows:minmax(0,1fr) auto}
  .deck .stickPanel:first-child{grid-column:1;grid-row:1}
  .deck .centerPanel{grid-column:1 / -1;grid-row:2;grid-template-columns:1fr 1fr;grid-template-rows:auto auto}
  .deck .stickPanel:last-child{grid-column:2;grid-row:1}
  .stickPanel{min-height:145px}
  .gimbal{width:min(42vw,23svh,176px);height:min(42vw,23svh,176px);min-width:112px;min-height:112px}
  .telemetry{grid-column:1 / -1;grid-template-columns:repeat(4,minmax(0,1fr))}
  .deadman{grid-column:1;min-height:34px}
  .trimGrid{grid-column:2}
  .monitor{grid-template-columns:repeat(4,minmax(0,1fr))}
  .controlStrip{grid-template-columns:minmax(100px,1.4fr) 48px minmax(92px,1fr) 48px}
}
@media (max-height:430px){
  .app{gap:3px;padding:4px max(4px,env(safe-area-inset-right)) max(4px,env(safe-area-inset-bottom)) max(4px,env(safe-area-inset-left))}
  .title{font-size:11px}
  .chip{height:20px;padding:0 5px;font-size:9px}
  .seg button{height:18px;min-width:36px;font-size:9px}
  .switches{gap:3px}
  .sw{height:25px}
  .sw b{font-size:9px}
  .sw span{display:none}
  .deck{grid-template-columns:minmax(112px,1fr) minmax(100px,.65fr) minmax(112px,1fr);gap:4px}
  .stickPanel{min-height:120px}
  .stickTitle{font-size:9px;left:6px;top:5px}
  .axisLabel{display:none}
  .gimbal{width:min(31svh,28vw,158px);height:min(31svh,28vw,158px);min-width:104px;min-height:104px}
  .centerPanel{padding:4px;gap:3px}
  .telemetry{gap:3px}
  .meter{padding:3px}
  .meter span{font-size:6px}
  .meter b{font-size:10px}
  .deadman{min-height:30px;font-size:9px}
  .miniBtn{height:23px;font-size:9px}
  .servoControl,.modeBtn,.taskBtn,.taskSelect{height:24px}
  .monitor{min-height:38px;padding:3px;gap:3px;grid-template-columns:repeat(8,minmax(0,1fr))}
  .bar label{font-size:6px}
  .track{height:5px}
}
</style>
</head>
<body>
<main class="app">
  <section class="top">
    <div class="title">RadioMaster Phone RC</div>
    <div class="chips">
      <div class="chip" id="linkChip"><i class="dot"></i><span id="linkText">OFFLINE</span></div>
      <div class="chip" id="sendChip"><i class="dot"></i><span id="sendText">SAFE</span></div>
      <div class="chip"><span id="rateText">50 Hz</span></div>
    </div>
    <div class="seg routeSeg">
      <button id="routeTrainer" class="active">TRAINER</button>
      <button id="routeDirect">DIRECT</button>
    </div>
    <div class="seg protoSeg">
      <button id="protoCrsf">CRSF</button>
      <button id="protoSbus" class="active">SBUS</button>
    </div>
  </section>

  <section class="switches">
    <button class="sw warn" data-toggle="master"><b>TRN</b><span>PHONE</span></button>
    <button class="sw warn" data-toggle="arm"><b>SA</b><span>ARM</span></button>
    <button class="sw on" data-toggle="angle"><b>SB</b><span>ANGLE</span></button>
    <button class="sw" data-toggle="air"><b>SC</b><span>AIR</span></button>
    <button class="sw moment" data-moment="beep"><b>SD</b><span>BEEP</span></button>
    <button class="sw" data-toggle="aux5"><b>SE</b><span>AUX5</span></button>
    <button class="sw" data-toggle="aux6"><b>SF</b><span>AUX6</span></button>
    <button class="sw" data-toggle="aux7"><b>SG</b><span>AUX7</span></button>
    <button class="sw" data-toggle="aux8"><b>SH</b><span>AUX8</span></button>
    <button class="sw warn" data-toggle="cut"><b>CUT</b><span>THR</span></button>
  </section>

  <section class="deck">
    <div class="stickPanel">
      <div class="stickTitle">LEFT</div>
      <div class="axisLabel top">THR+</div><div class="axisLabel bottom">THR-</div>
      <div class="axisLabel left">YAW-</div><div class="axisLabel right">YAW+</div>
      <div class="gimbal" id="leftStick"><div class="ring"></div><div class="knob"></div></div>
    </div>

    <div class="centerPanel">
      <div class="telemetry">
        <div class="meter"><span>OUTPUT</span><b id="outState">safe</b></div>
        <div class="meter"><span>LINK AGE</span><b id="ageState">--</b></div>
        <div class="meter"><span>ROUTE</span><b id="routeState">TRAINER</b></div>
        <div class="meter"><span>FRAMES</span><b id="frameState">0</b></div>
      </div>
      <button class="deadman" id="deadman">PHONE CONTROL OFF</button>
      <div class="trimGrid">
        <button class="miniBtn" id="directConfirm">CONFIRM DIRECT</button>
        <button class="miniBtn" id="centerSticks">CENTER</button>
        <button class="miniBtn" id="zeroThrottle">THR LOW</button>
      </div>
    </div>

    <div class="stickPanel">
      <div class="stickTitle">RIGHT</div>
      <div class="axisLabel top">PITCH+</div><div class="axisLabel bottom">PITCH-</div>
      <div class="axisLabel left">ROLL-</div><div class="axisLabel right">ROLL+</div>
      <div class="gimbal" id="rightStick"><div class="ring"></div><div class="knob"></div></div>
    </div>
  </section>

  <section class="controlStrip">
    <label class="servoControl"><span>SERVO</span><input id="servoRange" type="range" min="0" max="180" step="1" value="90"><b id="servoValue">90</b></label>
    <button class="modeBtn" data-toggle="hover" id="hoverMode">HOVER</button>
    <select class="taskSelect" id="taskSelect" aria-label="Position task">
      <option value="1">FORWARD 1m</option>
      <option value="2">RIGHT 1m</option>
      <option value="3">HOME</option>
      <option value="4">TASK 1</option>
      <option value="5">TASK 2</option>
    </select>
    <button class="taskBtn" id="taskExecute">RUN</button>
  </section>

  <section class="monitor" id="monitor"></section>
</main>
<script>
(() => {
  const WS_PORT = 7778;
  const SEND_MS = 20;
  const MIN = 988, MID = 1500, MAX = 2012;
  const SPAN = (MAX - MIN) / 2;
  const TRAINER_NAMES = ["Roll","Pitch","Thr","Yaw","Arm","Mode","Air","Run","Task","Servo","Aux5","Aux6","Aux7","Aux8","Beep","Heartbeat"];
  const DIRECT_NAMES = ["Roll","Pitch","Thr","Yaw","Arm","Mode","Air","Run","Task","Servo","Param1","Param2","Param3","Param4","Beep","Reserve"];
  const state = {
    ws:null, connected:false, deadman:false, seq:0, serverFrames:0, serverErrors:0,
    route:"trainer", directConfirm:false, directReady:false, directActive:false, directAge:"--", directFrames:0, directErrors:0,
    left:{x:0,y:1}, right:{x:0,y:0}, servo:90, task:"1", taskExecute:false,
    sw:{master:false, arm:false, angle:true, hover:false, air:false, beep:false, aux5:false, aux6:false, aux7:false, aux8:false, cut:false}
  };

  const $ = (id) => document.getElementById(id);
  const clamp = (v,min,max) => Math.max(min, Math.min(max, v));
  const us = (v) => Math.round(clamp(v, MIN, MAX));
  const axisUs = (v) => us(MID + v * SPAN);
  const throttleUs = (y) => us(MID - y * SPAN);
  const highLow = (v) => v ? MAX : MIN;

  const monitor = $("monitor");
  const bars = [];
  for (let i=0;i<16;i++) {
    const row = document.createElement("div");
    row.className = "bar";
    row.innerHTML = `<label><span id="chn${i}">CH${i+1} ${DIRECT_NAMES[i]}</span><span id="chv${i}">1500</span></label><div class="track"><div class="fill" id="chf${i}"></div></div>`;
    monitor.appendChild(row);
    bars.push({n:$("chn"+i), v:$("chv"+i), f:$("chf"+i)});
  }

  document.addEventListener("contextmenu", e => e.preventDefault());
  document.addEventListener("selectstart", e => e.preventDefault());

  class Stick {
    constructor(el, data, opts) {
      this.el = el;
      this.knob = el.querySelector(".knob");
      this.data = data;
      this.opts = opts || {};
      this.pointer = null;
      el.addEventListener("pointerdown", e => this.down(e));
      el.addEventListener("pointermove", e => this.move(e));
      el.addEventListener("pointerup", e => this.up(e));
      el.addEventListener("pointercancel", e => this.up(e));
      this.render();
    }
    down(e){ this.pointer = e.pointerId; this.el.setPointerCapture(e.pointerId); this.move(e); }
    move(e){
      if (this.pointer !== e.pointerId) return;
      const r = this.el.getBoundingClientRect();
      const cx = r.left + r.width/2, cy = r.top + r.height/2;
      const radius = Math.min(r.width, r.height) * 0.39;
      const dx = clamp((e.clientX - cx) / radius, -1, 1);
      const dy = clamp((e.clientY - cy) / radius, -1, 1);
      this.data.x = dx;
      this.data.y = dy;
      this.render();
    }
    up(e){
      if (this.pointer !== e.pointerId) return;
      this.pointer = null;
      if (this.opts.centerX !== false) this.data.x = 0;
      if (this.opts.centerY !== false) this.data.y = 0;
      this.render();
    }
    render(){
      this.knob.style.transform = `translate(calc(-50% + ${this.data.x*118}%), calc(-50% + ${this.data.y*118}%))`;
    }
  }

  const leftStick = new Stick($("leftStick"), state.left, {centerY:false});
  const rightStick = new Stick($("rightStick"), state.right, {});

  function channels(){
    // Throttle cut must always command the minimum pulse. Sending MAX here
    // makes the FC report THROTTLE and is unsafe in both trainer and direct
    // routes.
    const throttle = state.sw.cut ? MIN : throttleUs(state.left.y);
    const arm = state.sw.cut ? false : state.sw.arm;
    const servo = us(MIN + (state.servo / 180) * (MAX - MIN));
    const mode = state.sw.hover ? MAX : (state.sw.angle ? MID : MIN);
    const taskSelector = {"1":1200,"2":1400,"3":1600,"4":1800,"5":2000}[state.task] || 1000;
    const ch = new Array(16).fill(1500);
    ch[0] = axisUs(state.right.x);
    ch[1] = axisUs(-state.right.y);
    ch[2] = throttle;
    ch[3] = axisUs(state.left.x);
    ch[4] = highLow(arm);
    if (state.route === "trainer") {
      // EdgeTX Trainer -> Chans replaces the RadioMaster outputs directly, so
      // trainer data must use the same CH1-CH16 layout as the receiver.
      ch[5] = mode;
      ch[6] = highLow(state.sw.air);
      ch[7] = highLow(state.taskExecute && state.sw.master);
      ch[8] = taskSelector;
      ch[9] = servo;
      ch[10] = highLow(state.sw.aux5);
      ch[11] = highLow(state.sw.aux6);
      ch[12] = highLow(state.sw.aux7);
      ch[13] = highLow(state.sw.aux8);
      ch[14] = highLow(state.sw.beep);
      ch[15] = 1250;
    } else {
      ch[5] = mode;
      ch[6] = highLow(state.sw.air);
      ch[7] = highLow(state.taskExecute && state.sw.master);
      ch[8] = taskSelector;
      ch[9] = servo;
      ch[10] = highLow(state.sw.aux5);
      ch[11] = highLow(state.sw.aux6);
      ch[12] = highLow(state.sw.aux7);
      ch[13] = highLow(state.sw.aux8);
      ch[14] = highLow(state.sw.beep);
    }
    return ch;
  }

  function updateBars(ch){
    for (let i=0;i<16;i++) {
      bars[i].n.textContent = state.route === "trainer" ? `TR${i+1}->CH${i+1} ${TRAINER_NAMES[i]}` : `CH${i+1} ${DIRECT_NAMES[i]}`;
      bars[i].v.textContent = ch[i];
      bars[i].f.style.width = `${clamp((ch[i]-1000)/10, 0, 100)}%`;
    }
  }

  function setChip(el, ok){ el.classList.toggle("ok", !!ok); }

  function updateUi(){
    const ch = channels();
    const trainerActive = state.connected && state.sw.master && state.route === "trainer";
    const directActive = state.connected && state.sw.master && state.route === "direct" && state.directConfirm;
    const active = trainerActive || directActive;
    updateBars(ch);
    setChip($("linkChip"), state.connected);
    setChip($("sendChip"), active);
    $("linkText").textContent = state.connected ? "ONLINE" : "OFFLINE";
    $("sendText").textContent = directActive ? "DIRECT" : (trainerActive ? "TRAINER" : "RADIO");
    $("outState").textContent = directActive ? "direct" : (trainerActive ? "trainer" : "radio");
    $("outState").className = active ? "good" : "";
    $("routeState").textContent = state.route === "direct" ? (state.directConfirm ? "DIRECT OK" : "DIRECT ?") : "TRAINER";
    $("routeState").className = directActive ? "good" : (state.route === "direct" ? "warnText" : "");
    $("frameState").textContent = state.route === "direct" ? (state.directFrames || 0) : (state.serverFrames || state.seq);
    $("routeTrainer").classList.toggle("active", state.route === "trainer");
    $("routeDirect").classList.toggle("active", state.route === "direct");
    $("directConfirm").classList.toggle("danger", state.route === "direct" && !state.directConfirm);
    $("directConfirm").classList.toggle("good", state.route === "direct" && state.directConfirm);
    $("directConfirm").textContent = state.route === "direct" ? (state.directConfirm ? "DIRECT CONFIRMED" : "ARE YOU SURE?") : "CONFIRM DIRECT";
    $("deadman").classList.toggle("on", state.sw.master);
    $("deadman").textContent = state.sw.master ? "PHONE ON" : "PHONE OFF";
    $("servoValue").textContent = state.servo;
    $("hoverMode").textContent = state.sw.hover ? "HOLD" : (state.sw.angle ? "ANGLE" : "ACRO");
    $("taskExecute").classList.toggle("on", state.taskExecute);
    for (const [key, value] of Object.entries(state.sw)) {
      document.querySelectorAll(`[data-toggle="${key}"]`).forEach(btn => btn.classList.toggle("on", value));
    }
    document.querySelectorAll("[data-moment='beep']").forEach(btn => btn.classList.toggle("on", state.sw.beep));
  }

  function sendFrame(){
    const ch = channels();
    if (!state.ws || state.ws.readyState !== WebSocket.OPEN) return;
    const trainer = state.route === "trainer" && state.sw.master ? 1 : 0;
    const direct = state.route === "direct" && state.sw.master && state.directConfirm ? 1 : 0;
    const confirm = state.route === "direct" && state.directConfirm ? 1 : 0;
    state.ws.send(["P", state.seq++, trainer, direct, confirm, ...ch].join(","));
  }

  function sendCommand(line){
    if (state.ws && state.ws.readyState === WebSocket.OPEN) state.ws.send("M," + line);
  }

  function connect(){
    const ws = new WebSocket(`ws://${location.hostname}:${WS_PORT}/rc`);
    state.ws = ws;
    ws.onopen = () => { state.connected = true; updateUi(); ws.send("Q"); };
    ws.onclose = () => {
      state.connected = false;
      state.deadman = false;
      state.sw.master = false;
      state.sw.arm = false;
      state.taskExecute = false;
      state.directConfirm = false;
      updateUi();
      setTimeout(connect, 700);
    };
    ws.onerror = () => { try { ws.close(); } catch(e) {} };
    ws.onmessage = (ev) => handleStatus(String(ev.data));
  }

  function handleStatus(text){
    if (!text.startsWith("S,")) return;
    const p = text.split(",");
    $("ageState").textContent = `${p[3] || "--"} ms`;
    $("protoCrsf").classList.toggle("active", p[5] === "CRSF");
    $("protoSbus").classList.toggle("active", p[5] === "SBUS");
    state.serverFrames = Number(p[7] || 0);
    state.serverErrors = Number(p[8] || 0);
    state.directReady = p[9] === "1";
    state.directActive = p[10] === "1";
    state.directAge = p[11] || "--";
    state.directFrames = Number(p[12] || 0);
    state.directErrors = Number(p[13] || 0);
    updateUi();
  }

  document.querySelectorAll("[data-toggle]").forEach(btn => {
    btn.addEventListener("click", () => {
      const key = btn.dataset.toggle;
      state.sw[key] = !state.sw[key];
      if (key === "master" && state.route === "direct" && state.sw.master && !state.directConfirm) state.sw.master = false;
      if (key === "master" && !state.sw.master) state.sw.arm = false;
      if (key === "master" && !state.sw.master) state.taskExecute = false;
      if (key === "cut" && state.sw.cut) state.sw.arm = false;
      if (key === "hover" && state.sw.hover) state.sw.angle = true;
      if (key === "angle" && !state.sw.angle) state.sw.hover = false;
      updateUi();
    });
  });

  document.querySelectorAll("[data-moment]").forEach(btn => {
    const key = btn.dataset.moment;
    const on = (e) => { e.preventDefault(); state.sw[key] = true; updateUi(); };
    const off = (e) => { e.preventDefault(); state.sw[key] = false; updateUi(); };
    btn.addEventListener("pointerdown", on);
    btn.addEventListener("pointerup", off);
    btn.addEventListener("pointercancel", off);
    btn.addEventListener("pointerleave", off);
  });

  $("deadman").addEventListener("click", e => {
    e.preventDefault();
    if (state.route === "direct" && !state.directConfirm) state.sw.master = false;
    else state.sw.master = !state.sw.master;
    if (!state.sw.master) { state.sw.arm = false; state.taskExecute = false; }
    updateUi();
  });
  $("routeTrainer").addEventListener("click", () => { state.route = "trainer"; state.directConfirm = false; state.sw.master = false; state.sw.arm = false; state.taskExecute = false; updateUi(); sendFrame(); });
  $("routeDirect").addEventListener("click", () => { state.route = "direct"; state.directConfirm = false; state.sw.master = false; state.sw.arm = false; state.taskExecute = false; updateUi(); sendFrame(); });
  $("directConfirm").addEventListener("click", () => { if (state.route === "direct") { state.directConfirm = !state.directConfirm; if (!state.directConfirm) { state.sw.master = false; state.sw.arm = false; state.taskExecute = false; } updateUi(); sendFrame(); } });
  $("centerSticks").addEventListener("click", () => { state.left.x = 0; state.right.x = 0; state.right.y = 0; leftStick.render(); rightStick.render(); updateUi(); });
  $("zeroThrottle").addEventListener("click", () => { state.left.y = 1; state.sw.cut = true; state.sw.arm = false; leftStick.render(); updateUi(); });
  $("servoRange").addEventListener("input", e => { state.servo = clamp(Number(e.target.value), 0, 180); updateUi(); });
  $("taskSelect").addEventListener("change", e => { state.task = String(e.target.value); updateUi(); });
  const taskOn = e => { e.preventDefault(); if (state.sw.master && state.sw.hover) { state.taskExecute = true; updateUi(); sendFrame(); } };
  const taskOff = e => { e.preventDefault(); if (state.taskExecute) { state.taskExecute = false; updateUi(); sendFrame(); } };
  $("taskExecute").addEventListener("pointerdown", taskOn);
  $("taskExecute").addEventListener("pointerup", taskOff);
  $("taskExecute").addEventListener("pointercancel", taskOff);
  $("taskExecute").addEventListener("pointerleave", taskOff);
  $("protoCrsf").addEventListener("click", () => sendCommand("PROTO CRSF"));
  $("protoSbus").addEventListener("click", () => sendCommand("PROTO SBUS"));

  document.addEventListener("visibilitychange", () => {
    if (document.hidden) {
      state.deadman = false;
      state.sw.master = false;
      state.sw.arm = false;
      state.taskExecute = false;
      state.directConfirm = false;
      sendFrame();
    }
  });

  window.addEventListener("beforeunload", () => {
    state.deadman = false;
    state.sw.master = false;
    state.sw.arm = false;
    state.taskExecute = false;
    state.directConfirm = false;
    sendFrame();
  });

  setInterval(() => { sendFrame(); updateUi(); }, SEND_MS);
  updateUi();
  connect();
})();
</script>
</body>
</html>)APPHTML";

const char kManifestJson[] =
    R"JSON({"name":"RadioMaster Phone RC","short_name":"Phone RC","start_url":"/","display":"standalone","background_color":"#101318","theme_color":"#101318"})JSON";

bool isRequestForPath(const char *request, const char *path) {
  if (request == nullptr || path == nullptr) return false;
  if (strncmp(request, "GET ", 4) != 0) return false;
  const char *start = request + 4;
  const char *end = strchr(start, ' ');
  if (end == nullptr) return false;
  const size_t len = static_cast<size_t>(end - start);
  return strlen(path) == len && strncmp(start, path, len) == 0;
}

bool lineStartsWithIgnoreCase(const char *line, const char *prefix) {
  while (*prefix) {
    if (*line == '\0') return false;
    if (toupper(static_cast<unsigned char>(*line)) !=
        toupper(static_cast<unsigned char>(*prefix))) {
      return false;
    }
    ++line;
    ++prefix;
  }
  return true;
}

bool getHeaderValue(const char *request, const char *headerName, char *out, size_t outCapacity) {
  if (request == nullptr || headerName == nullptr || out == nullptr || outCapacity == 0) return false;
  const char *line = request;
  while (*line) {
    const char *next = strstr(line, "\r\n");
    const size_t lineLen = next ? static_cast<size_t>(next - line) : strlen(line);
    char local[160];
    const size_t copyLen = lineLen < sizeof(local) - 1 ? lineLen : sizeof(local) - 1;
    memcpy(local, line, copyLen);
    local[copyLen] = '\0';

    if (lineStartsWithIgnoreCase(local, headerName)) {
      const char *value = local + strlen(headerName);
      while (*value == ' ' || *value == '\t') ++value;
      strncpy(out, value, outCapacity);
      out[outCapacity - 1] = '\0';
      return true;
    }

    if (!next) break;
    line = next + 2;
  }
  return false;
}

bool buildWebSocketAccept(const char *key, char *out, size_t outCapacity) {
  if (key == nullptr || out == nullptr || outCapacity == 0) return false;

  char joined[128];
  snprintf(joined, sizeof(joined), "%s%s", key, kWebSocketGuid);

  unsigned char sha[20];
#if MBEDTLS_VERSION_NUMBER >= 0x03000000
  if (mbedtls_sha1(reinterpret_cast<const unsigned char *>(joined), strlen(joined), sha) != 0) return false;
#else
  if (mbedtls_sha1_ret(reinterpret_cast<const unsigned char *>(joined), strlen(joined), sha) != 0) return false;
#endif

  size_t written = 0;
  if (mbedtls_base64_encode(reinterpret_cast<unsigned char *>(out), outCapacity, &written, sha, sizeof(sha)) != 0) {
    return false;
  }
  if (written >= outCapacity) return false;
  out[written] = '\0';
  return true;
}

bool parseUintToken(char *token, uint32_t &value) {
  if (token == nullptr || *token == '\0') return false;
  char *end = nullptr;
  const unsigned long parsed = strtoul(token, &end, 10);
  if (end == token || *end != '\0') return false;
  value = static_cast<uint32_t>(parsed);
  return true;
}

}  // namespace

WebAppServer::WebAppServer(CommandProcessor &commands, ChannelState &channels, DirectRcLink &directRc)
    : commands_(commands),
      channels_(channels),
      directRc_(directRc),
      httpServer_(Config::WifiHttpPort),
      wsServer_(Config::WifiWebSocketPort) {}

void WebAppServer::begin(Print &log) {
  httpServer_.begin();
  wsServer_.begin();
  httpServer_.setNoDelay(true);
  wsServer_.setNoDelay(true);

  log.print(F("Web app: http://"));
  log.println(WiFi.softAPIP());
  log.print(F("WebSocket endpoint: ws://"));
  log.print(WiFi.softAPIP());
  log.print(':');
  log.println(Config::WifiWebSocketPort);
}

void WebAppServer::poll(Print &log) {
  pollHttp(log);
  pollWebSocket(log);

  const uint32_t now = millis();
  if (wsReady_ && now - lastStatusMs_ >= Config::PhoneStatusIntervalMs) {
    lastStatusMs_ = now;
    sendStatus();
  }
}

void WebAppServer::pollHttp(Print &log) {
  WiFiClient client = httpServer_.available();
  if (!client) return;

  client.setTimeout(30);
  char request[kHttpRequestMax] = {};
  if (!readHttpRequest(client, request, sizeof(request), kHttpReadTimeoutMs)) {
    client.stop();
    return;
  }

  if (isRequestForPath(request, "/") || isRequestForPath(request, "/index.html")) {
    serveIndex(client);
  } else if (isRequestForPath(request, "/manifest.json")) {
    serveManifest(client);
  } else {
    serveNotFound(client);
  }

  delay(1);
  client.stop();
  (void)log;
}

void WebAppServer::pollWebSocket(Print &log) {
  acceptWebSocket(log);
  if (!wsReady_ || !wsClient_ || !wsClient_.connected()) {
    closeWebSocket();
    return;
  }

  for (uint8_t i = 0; i < 4 && wsClient_.available() > 0; ++i) {
    char message[kWsMessageMax] = {};
    uint8_t opcode = 0;
    if (!readWebSocketText(message, sizeof(message), opcode)) {
      break;
    }

    if (opcode == 0x8) {
      closeWebSocket();
      return;
    }
    if (opcode == 0x9) {
      sendWebSocketControl(0xA, nullptr, 0);
      continue;
    }
    if (opcode == 0x1) {
      handleWebSocketMessage(message, log);
    }
  }
}

void WebAppServer::acceptWebSocket(Print &log) {
  WiFiClient next = wsServer_.available();
  if (!next) return;

  next.setTimeout(40);
  char request[kHttpRequestMax] = {};
  if (!readHttpRequest(next, request, sizeof(request), kWsHandshakeTimeoutMs) ||
      !handleWebSocketHandshake(next, request, log)) {
    next.stop();
    return;
  }

  if (wsClient_ && wsClient_.connected()) {
    sendWebSocketText("E,replaced");
    wsClient_.stop();
  }

  wsClient_ = next;
  wsClient_.setNoDelay(true);
  wsClient_.setTimeout(4);
  wsReady_ = true;
  lastStatusMs_ = 0;
  log.print(F("Web app client connected from "));
  log.println(wsClient_.remoteIP());
  sendStatus();
}

void WebAppServer::closeWebSocket() {
  if (wsClient_) {
    wsClient_.stop();
  }
  wsReady_ = false;
}

bool WebAppServer::readHttpRequest(WiFiClient &client, char *request, size_t capacity, uint32_t timeoutMs) {
  if (request == nullptr || capacity == 0) return false;
  size_t length = 0;
  const uint32_t deadline = millis() + timeoutMs;

  while (client.connected() && static_cast<int32_t>(millis() - deadline) < 0) {
    while (client.available() > 0) {
      const int value = client.read();
      if (value < 0) break;
      if (length + 1 < capacity) {
        request[length++] = static_cast<char>(value);
        request[length] = '\0';
      }
      if (length >= 4 && strstr(request, "\r\n\r\n") != nullptr) {
        return true;
      }
    }
    delay(1);
  }

  return length > 0;
}

void WebAppServer::serveIndex(WiFiClient &client) {
  client.print(F("HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/html; charset=utf-8\r\n"
                 "Cache-Control: no-store\r\n"
                 "Connection: close\r\n\r\n"));
  client.print(kIndexHtml);
}

void WebAppServer::serveManifest(WiFiClient &client) {
  client.print(F("HTTP/1.1 200 OK\r\n"
                 "Content-Type: application/manifest+json\r\n"
                 "Cache-Control: no-store\r\n"
                 "Connection: close\r\n\r\n"));
  client.print(kManifestJson);
}

void WebAppServer::serveNotFound(WiFiClient &client) {
  client.print(F("HTTP/1.1 404 Not Found\r\n"
                 "Content-Type: text/plain\r\n"
                 "Connection: close\r\n\r\n"
                 "not found\r\n"));
}

bool WebAppServer::handleWebSocketHandshake(WiFiClient &client, const char *request, Print &log) {
  if (!isRequestForPath(request, "/rc")) {
    client.print(F("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n"));
    return false;
  }

  char key[96] = {};
  if (!getHeaderValue(request, "Sec-WebSocket-Key:", key, sizeof(key))) {
    client.print(F("HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n"));
    return false;
  }

  char accept[48] = {};
  if (!buildWebSocketAccept(key, accept, sizeof(accept))) {
    client.print(F("HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n"));
    log.println(F("ERR websocket accept build failed"));
    return false;
  }

  client.print(F("HTTP/1.1 101 Switching Protocols\r\n"
                 "Upgrade: websocket\r\n"
                 "Connection: Upgrade\r\n"
                 "Sec-WebSocket-Accept: "));
  client.print(accept);
  client.print(F("\r\n\r\n"));
  return true;
}

bool WebAppServer::readWebSocketText(char *message, size_t capacity, uint8_t &opcode) {
  if (!wsClient_ || !wsClient_.connected() || message == nullptr || capacity == 0) return false;
  if (wsClient_.available() < 2) return false;

  uint8_t header[2] = {};
  if (wsClient_.readBytes(header, sizeof(header)) != sizeof(header)) return false;

  opcode = header[0] & 0x0F;
  const bool masked = (header[1] & 0x80) != 0;
  uint64_t length = header[1] & 0x7F;

  if (length == 126) {
    uint8_t ext[2] = {};
    if (wsClient_.readBytes(ext, sizeof(ext)) != sizeof(ext)) return false;
    length = (static_cast<uint16_t>(ext[0]) << 8) | ext[1];
  } else if (length == 127) {
    closeWebSocket();
    return false;
  }

  if (length >= capacity) {
    closeWebSocket();
    return false;
  }

  uint8_t mask[4] = {};
  if (masked && wsClient_.readBytes(mask, sizeof(mask)) != sizeof(mask)) return false;

  for (uint64_t i = 0; i < length; ++i) {
    int value = wsClient_.read();
    if (value < 0) {
      uint8_t byte = 0;
      if (wsClient_.readBytes(&byte, 1) != 1) return false;
      value = byte;
    }
    uint8_t data = static_cast<uint8_t>(value);
    if (masked) data ^= mask[i & 0x03];
    message[i] = static_cast<char>(data);
  }
  message[length] = '\0';
  return true;
}

bool WebAppServer::sendWebSocketText(const char *message) {
  if (!wsReady_ || !wsClient_ || !wsClient_.connected() || message == nullptr) return false;
  return sendWebSocketControl(0x1, reinterpret_cast<const uint8_t *>(message), strlen(message));
}

bool WebAppServer::sendWebSocketControl(uint8_t opcode, const uint8_t *payload, size_t length) {
  if (!wsReady_ || !wsClient_ || !wsClient_.connected()) return false;
  if (length > 65535) return false;

  uint8_t header[4] = {};
  size_t headerLength = 0;
  header[0] = 0x80 | (opcode & 0x0F);
  if (length < 126) {
    header[1] = static_cast<uint8_t>(length);
    headerLength = 2;
  } else {
    header[1] = 126;
    header[2] = static_cast<uint8_t>((length >> 8) & 0xFF);
    header[3] = static_cast<uint8_t>(length & 0xFF);
    headerLength = 4;
  }

  if (wsClient_.write(header, headerLength) != headerLength) return false;
  if (length == 0) return true;
  return wsClient_.write(payload, length) == length;
}

void WebAppServer::handleWebSocketMessage(const char *message, Print &log) {
  if (message == nullptr || *message == '\0') return;

  if (strcmp(message, "Q") == 0) {
    sendStatus();
    return;
  }

  if (strncmp(message, "M,", 2) == 0) {
    commands_.handleLine(message + 2, log);
    sendStatus();
    return;
  }

  uint16_t channelsUs[Config::ChannelCount] = {};
  bool trainerEnabled = false;
  bool directEnabled = false;
  bool directConfirmed = false;
  if (!parsePhoneFrame(message, channelsUs, trainerEnabled, directEnabled, directConfirmed)) {
    phoneErrors_++;
    return;
  }

  const uint32_t now = millis();
  if (directEnabled) {
    commands_.stopPhoneControl(now);
  } else {
    commands_.applyPhoneFrame(channelsUs, trainerEnabled, trainerEnabled, now);
  }
  // Trainer frames are replaced inside EdgeTX and then travel through ELRS.
  // ESP-NOW is reserved for explicitly confirmed direct takeover only.
  directRc_.applyPhoneFrame(channelsUs, false, directEnabled, directConfirmed, now);
  phoneFrames_++;
}

bool WebAppServer::parsePhoneFrame(const char *message,
                                   uint16_t channelsUs[Config::ChannelCount],
                                   bool &trainerEnabled,
                                   bool &directEnabled,
                                   bool &directConfirmed) {
  if (message == nullptr || channelsUs == nullptr) return false;

  char editable[kWsMessageMax] = {};
  strncpy(editable, message, sizeof(editable));
  editable[sizeof(editable) - 1] = '\0';

  char *save = nullptr;
  char *token = strtok_r(editable, ",", &save);
  if (token == nullptr || strcmp(token, "P") != 0) return false;

  uint32_t parsed = 0;
  token = strtok_r(nullptr, ",", &save);  // sequence
  if (!parseUintToken(token, parsed)) return false;

  token = strtok_r(nullptr, ",", &save);
  if (!parseUintToken(token, parsed)) return false;
  trainerEnabled = parsed != 0;

  token = strtok_r(nullptr, ",", &save);
  if (!parseUintToken(token, parsed)) return false;
  directEnabled = parsed != 0;

  token = strtok_r(nullptr, ",", &save);
  if (!parseUintToken(token, parsed)) return false;
  directConfirmed = parsed != 0;

  for (size_t i = 0; i < Config::ChannelCount; ++i) {
    token = strtok_r(nullptr, ",", &save);
    if (!parseUintToken(token, parsed)) return false;
    if (parsed < Config::ChannelMinUs || parsed > Config::ChannelMaxUs) return false;
    channelsUs[i] = static_cast<uint16_t>(parsed);
  }

  return true;
}

void WebAppServer::sendStatus() {
  const uint32_t now = millis();
  char status[420];
  size_t used = snprintf(status,
                         sizeof(status),
                         "S,%lu,%u,%lu,%u,%s,%u,%lu,%lu,%u,%u,%lu,%lu,%lu",
                         static_cast<unsigned long>(now),
                         commands_.outputEnabled() ? 1 : 0,
                         static_cast<unsigned long>(commands_.lastHostMs() ? now - commands_.lastHostMs() : 0),
                         commands_.linkFresh(now) ? 1 : 0,
                         radioProtocolName(commands_.protocol()),
                         commands_.outputRateHz(),
                         static_cast<unsigned long>(phoneFrames_),
                         static_cast<unsigned long>(phoneErrors_),
                         directRc_.ready() ? 1 : 0,
                         directRc_.active(now) ? 1 : 0,
                         static_cast<unsigned long>(directRc_.ageMs(now)),
                         static_cast<unsigned long>(directRc_.sentFrames()),
                         static_cast<unsigned long>(directRc_.sendErrors()));
  if (used >= sizeof(status)) return;

  for (size_t i = 0; i < Config::ChannelCount && used < sizeof(status); ++i) {
    used += snprintf(status + used, sizeof(status) - used, ",%u", channels_.getUs(i));
  }

  sendWebSocketText(status);
}
