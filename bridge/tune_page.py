PAGE = """<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Cover tuning</title>
<style>
  body { font: 15px system-ui, sans-serif; background: #15171c; color: #e6e8ee; margin: 0; padding: 16px; max-width: 760px; }
  h1 { font-size: 20px; margin: 0 0 4px; }
  p { color: #9aa1b2; margin: 4px 0 14px; }
  .row { display: grid; grid-template-columns: 110px 1fr 70px 34px; gap: 10px; align-items: center; margin: 8px 0; }
  .row label { color: #c9ceda; }
  input[type=range] { width: 100%; }
  input[type=number] { width: 70px; background: #1f232b; color: inherit; border: 1px solid #343a46; border-radius: 4px; padding: 3px; }
  button { background: #262b35; color: inherit; border: 1px solid #3a4150; border-radius: 4px; padding: 4px 8px; cursor: pointer; }
  button:hover { background: #323847; }
  .bar { display: flex; gap: 12px; align-items: center; margin: 14px 0; }
  .pics { display: flex; gap: 16px; flex-wrap: wrap; margin: 12px 0; }
  figure { margin: 0; }
  figure img { image-rendering: pixelated; width: 264px; height: 264px; background: #000; display: block; }
  figcaption { color: #9aa1b2; margin-top: 4px; }
  textarea { width: 100%; box-sizing: border-box; background: #1f232b; color: inherit; border: 1px solid #343a46; border-radius: 4px; padding: 8px; font: 13px ui-monospace, monospace; }
  #status { color: #9aa1b2; }
</style>
</head>
<body>
<h1>Cover tuning</h1>
<p>Drag a value and watch the board: with the Music page open, the cover on the panel refreshes within one poll (set Settings &gt; Connections &gt; Spotify &gt; Poll every to 5 s). This screen is not the panel, so judge by the board.</p>
<div id="rows"></div>
<div class="bar"><button id="reset">Reset to .env / defaults</button><span id="status"></span></div>
<div class="pics">
  <figure><img id="orig" alt=""><figcaption>Original (88 x 88 crop)</figcaption></figure>
  <figure><img id="fixed" alt=""><figcaption>Sent to the device</figcaption></figure>
</div>
<p>Values in use. They are saved by the bridge in data/cover_tuning.json and survive restarts; to also keep them in <code>.env</code>, paste these lines there.</p>
<textarea id="env" rows="8" readonly></textarea>
<script>
const rows = document.getElementById('rows');
const statusText = document.getElementById('status');
const envText = document.getElementById('env');
let inputs = {};
let album = '';
let timer = null;

function label(name) {
  return name.replace('COVER_', '').toLowerCase().replace('_', ' ');
}

function build(state) {
  rows.textContent = '';
  inputs = {};
  state.params.forEach(function (p) {
    const row = document.createElement('div');
    row.className = 'row';
    const name = document.createElement('label');
    name.textContent = label(p.name);
    const slider = document.createElement('input');
    slider.type = 'range';
    const number = document.createElement('input');
    number.type = 'number';
    [slider, number].forEach(function (input) {
      input.min = p.low;
      input.max = p.high;
      input.step = 0.01;
      input.value = p.value;
    });
    const back = document.createElement('button');
    back.textContent = '\\u21BA';
    back.title = 'default ' + p.default;
    slider.addEventListener('input', function () { number.value = slider.value; schedule(); });
    number.addEventListener('change', function () { slider.value = number.value; schedule(); });
    back.addEventListener('click', function () { slider.value = number.value = p.default; schedule(); });
    row.append(name, slider, number, back);
    rows.appendChild(row);
    inputs[p.name] = number;
  });
  show(state);
}

function show(state) {
  album = state.album || '';
  envText.value = state.params.map(function (p) { return p.name + '=' + p.value; }).join('\\n');
  statusText.textContent = state.saved ? 'applied' : 'applied, but the bridge could not save to disk';
  pictures(state.rev);
}

function pictures(rev) {
  const base = album ? '/spotify/cover?id=' + encodeURIComponent(album) + '&scale=3&r=' + rev + '&fmt=' : '';
  document.getElementById('orig').src = base ? base + 'orig' : '';
  document.getElementById('fixed').src = base ? base + 'png' : '';
  if (!album) statusText.textContent += ' - nothing playing yet, so no preview';
}

function schedule() {
  statusText.textContent = '...';
  clearTimeout(timer);
  timer = setTimeout(send, 200);
}

async function post(body) {
  const response = await fetch('/spotify/tune', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(body)
  });
  return response.json();
}

async function send() {
  const body = {};
  Object.keys(inputs).forEach(function (name) { body[name] = parseFloat(inputs[name].value); });
  try {
    show(await post(body));
  } catch (error) {
    statusText.textContent = 'could not reach the bridge';
  }
}

document.getElementById('reset').addEventListener('click', async function () {
  try {
    build(await post({ reset: true }));
  } catch (error) {
    statusText.textContent = 'could not reach the bridge';
  }
});

async function load() {
  build(await (await fetch('/spotify/tune/state')).json());
}

setInterval(async function () {
  try {
    const state = await (await fetch('/spotify/tune/state')).json();
    if ((state.album || '') !== album) {
      album = state.album || '';
      pictures(state.rev);
    }
  } catch (error) {}
}, 5000);

load();
</script>
</body>
</html>
"""
