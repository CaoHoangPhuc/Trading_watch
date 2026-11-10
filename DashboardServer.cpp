#include "DashboardServer.h"

#include "Config.h"

namespace {
const char PAGE[] PROGMEM = R"HTML(
<!doctype html>
<html>
<head>
<meta charset="utf-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>Trading Watch</title>
<style>
:root{--bg:#0f1720;--card:#17212b;--muted:#93a4b7;--text:#e7edf4;--ok:#2bbf83;--warn:#f3b63f;--bad:#ff6b6b;--line:#294053}
*{box-sizing:border-box}body{margin:0;background:linear-gradient(120deg,#0f1720,#13293d);font-family:ui-sans-serif,system-ui,sans-serif;color:var(--text)}
.container{max-width:1100px;margin:0 auto;padding:18px}
.card{background:rgba(23,33,43,.92);border:1px solid var(--line);border-radius:12px;padding:14px;margin-bottom:12px}
h1{margin:0 0 10px;font-size:22px}.muted{color:var(--muted);font-size:13px}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}.row{display:flex;gap:8px;flex-wrap:wrap;align-items:center}
label{font-size:12px;color:var(--muted)}select,input,button{background:#0d151d;border:1px solid #324b61;color:var(--text);padding:8px;border-radius:8px}
button{cursor:pointer}button:hover{border-color:#4f7395}.badge{padding:4px 8px;border-radius:999px;font-size:12px}.ok{background:#17372a;color:#95f1c9}.bad{background:#3f1d1d;color:#ffb2b2}
pre{white-space:pre-wrap;word-break:break-word;background:#0d151d;border:1px solid #2f4357;border-radius:8px;padding:8px;max-height:220px;overflow:auto}
.small{font-size:12px}
@media (max-width:800px){.grid{grid-template-columns:1fr}}
</style>
</head>
<body>
<div class="container">
  <h1>Trading Watch (ESP32-S3)</h1>
  <div class="muted">Bybit Option Watch Mode (BTC)</div>

  <div class="card">
    <div class="row" style="justify-content:space-between">
      <div>Device status: <span id="status" class="badge bad">offline</span></div>
      <div class="small muted" id="selected">selected: -</div>
    </div>
  </div>

  <div class="card">
    <div class="grid">
      <div>
        <label>Expiry</label>
        <select id="expiry"></select>
      </div>
      <div>
        <label>Side</label>
        <select id="side">
          <option value="ALL">ALL</option>
          <option value="C">CALL</option>
          <option value="P">PUT</option>
        </select>
      </div>
      <div>
        <label>Strike Min</label>
        <input id="minStrike" type="number" step="0.01" placeholder="optional"/>
      </div>
      <div>
        <label>Strike Max</label>
        <input id="maxStrike" type="number" step="0.01" placeholder="optional"/>
      </div>
    </div>
    <div class="row" style="margin-top:10px">
      <button id="applyFilter">Apply Filter</button>
      <button id="refreshContracts">Refresh Contracts</button>
      <span class="muted small" id="contractsMeta"></span>
    </div>
    <div style="margin-top:10px">
      <label>Symbol</label>
      <select id="symbol"></select>
      <button id="selectSymbol">Select Symbol</button>
    </div>
  </div>

  <div class="card">
    <h3 style="margin-top:0">Ticker (1Hz)</h3>
    <pre id="ticker">Waiting for ticker...</pre>
  </div>

  <div class="card">
    <h3 style="margin-top:0">System Metrics (1Hz)</h3>
    <pre id="metrics">Waiting for metrics...</pre>
  </div>

  <div class="card">
    <h3 style="margin-top:0">Command Channel</h3>
    <div class="row">
      <input id="cmd" type="text" placeholder="type command, e.g. help" style="flex:1"/>
      <button id="sendCmd">Send</button>
    </div>
    <pre id="cmdResp">No command sent yet.</pre>
  </div>
</div>

<script>
const state={contracts:[],expiries:[],ws:null};
const el=id=>document.getElementById(id);

function connect(){
  const url=`ws://${location.hostname}:81/`;
  const ws=new WebSocket(url);
  state.ws=ws;
  ws.onopen=()=>setStatus(true);
  ws.onclose=()=>{setStatus(false);setTimeout(connect,1200)};
  ws.onmessage=(e)=>onMessage(e.data);
}

function setStatus(ok){
  const s=el('status');
  s.textContent=ok?'online':'offline';
  s.className='badge '+(ok?'ok':'bad');
}

function send(obj){ if(state.ws&&state.ws.readyState===1) state.ws.send(JSON.stringify(obj)); }

function onMessage(raw){
  let msg;
  try{msg=JSON.parse(raw);}catch{return;}
  if(msg.type==='watch_state'){
    el('selected').textContent=`selected: ${msg.selected_symbol||'-'} | bybit_ws: ${msg.bybit_ws_connected?'up':'down'}`;
  }
  if(msg.type==='contracts_list'){
    state.contracts=msg.contracts||[];
    state.expiries=msg.expiries||[];
    renderFilters();
    renderSymbols();
    el('contractsMeta').textContent=`contracts: ${state.contracts.length}`;
  }
  if(msg.type==='ticker_1hz'){
    el('ticker').textContent=JSON.stringify(msg,null,2);
  }
  if(msg.type==='system_metrics_1hz'){
    el('metrics').textContent=JSON.stringify(msg,null,2);
  }
  if(msg.type==='command_response'){
    el('cmdResp').textContent=JSON.stringify(msg,null,2);
  }
  if(msg.type==='error_event'){
    el('cmdResp').textContent=JSON.stringify(msg,null,2);
  }
}

function renderFilters(){
  const expiry=el('expiry');
  const prev=expiry.value;
  expiry.innerHTML='';
  const all=document.createElement('option');
  all.value='ALL'; all.textContent='ALL'; expiry.appendChild(all);
  for(const ex of state.expiries){
    const o=document.createElement('option'); o.value=ex; o.textContent=ex; expiry.appendChild(o);
  }
  expiry.value = state.expiries.includes(prev) ? prev : 'ALL';
}

function currentFilters(){
  const minRaw=el('minStrike').value.trim();
  const maxRaw=el('maxStrike').value.trim();
  return {
    expiry: el('expiry').value||'ALL',
    side: el('side').value||'ALL',
    min_strike: minRaw.length?Number(minRaw):null,
    max_strike: maxRaw.length?Number(maxRaw):null,
  };
}

function filteredContracts(){
  const f=currentFilters();
  return state.contracts.filter(c=>{
    if(f.expiry!=='ALL' && c.expiry!==f.expiry) return false;
    if(f.side!=='ALL' && c.side!==f.side) return false;
    if(f.min_strike!==null && Number(c.strike)<f.min_strike) return false;
    if(f.max_strike!==null && Number(c.strike)>f.max_strike) return false;
    return true;
  });
}

function renderSymbols(){
  const symbol=el('symbol');
  const list=filteredContracts();
  symbol.innerHTML='';
  for(const c of list){
    const o=document.createElement('option');
    o.value=c.symbol;
    o.textContent=`${c.symbol} | ${c.expiry} | ${c.side} | ${c.strike}`;
    symbol.appendChild(o);
  }
}

el('applyFilter').onclick=()=>{ renderSymbols(); send({type:'set_filters',...currentFilters()}); };
el('refreshContracts').onclick=()=>send({type:'refresh_contracts'});
el('selectSymbol').onclick=()=>{ const s=el('symbol').value; if(s) send({type:'select_contract',symbol:s}); };
el('sendCmd').onclick=()=>{ const c=el('cmd').value.trim(); if(c) send({type:'command_request',command:c}); };
el('cmd').addEventListener('keydown',(e)=>{
  if(e.key==='Enter'){
    e.preventDefault();
    el('sendCmd').click();
  }
});

connect();
</script>
</body>
</html>
)HTML";
}  // namespace

DashboardServer::DashboardServer() : http_(Config::DASHBOARD_HTTP_PORT), ws_(Config::DASHBOARD_WS_PORT) {}

void DashboardServer::begin() {
  http_.on("/", [this]() { handleRoot(); });
  http_.begin();

  ws_.begin();
  ws_.onEvent([this](uint8_t clientNum, WStype_t type, uint8_t* payload, size_t length) {
    onWsEvent(clientNum, type, payload, length);
  });
}

void DashboardServer::loop() {
  http_.handleClient();
  ws_.loop();
}

void DashboardServer::broadcastJson(const String& payload) {
  String mutablePayload = payload;
  ws_.broadcastTXT(mutablePayload);
}

void DashboardServer::setMessageHandler(std::function<void(const String&)> handler) {
  handler_ = handler;
}

void DashboardServer::handleRoot() { http_.send_P(200, "text/html", PAGE); }

void DashboardServer::onWsEvent(uint8_t, WStype_t type, uint8_t* payload, size_t length) {
  if (type != WStype_TEXT || !handler_) {
    return;
  }

  String msg;
  msg.reserve(length);
  for (size_t i = 0; i < length; ++i) {
    msg += static_cast<char>(payload[i]);
  }

  handler_(msg);
}
