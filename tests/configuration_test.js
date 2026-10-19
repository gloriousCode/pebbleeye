'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const root = path.join(__dirname, '..');
const plain = value => JSON.parse(JSON.stringify(value));
function bridge(stored = '{}') {
  const handlers = {}, messages = [], timers = [], urls = [];
  let storage = stored;
  const context = vm.createContext({console: {log() {}}, setTimeout: callback => timers.push(callback),
    localStorage: {getItem: () => storage, setItem: (key, value) => {assert.equal(key, 'gabbroeye-hud-options-v1'); storage = value;}},
    Pebble: {addEventListener: (name, callback) => {handlers[name] = callback;}, openURL: url => urls.push(url),
      sendAppMessage: (settings, success, failure) => messages.push({settings: plain(settings), success, failure})}});
  vm.runInContext(fs.readFileSync(path.join(root, 'src/js/pebble-js-app.js'), 'utf8'), context);
  return {context, handlers, messages, timers, urls, stored: () => JSON.parse(storage)};
}
function editor(options, hash) {
  const html = fs.readFileSync(path.join(root, 'resources/configure-pebbleeye.html'), 'utf8');
  const nodes = {};
  for (const match of html.matchAll(/id="([^"]+)"/g)) {
    assert.ok(!nodes[match[1]], 'unique element IDs');
    nodes[match[1]] = {value: '', checked: false, style: {}, attributes: {}, handlers: {},
      setAttribute(name, value) {this.attributes[name] = value;}, addEventListener(name, callback) {this.handlers[name] = callback;}};
  }
  const document = {getElementById: id => {assert.ok(nodes[id], id); return nodes[id];}, location: ''};
  const context = vm.createContext({document, window: {location: {hash: hash === undefined ? '#options=' + encodeURIComponent(JSON.stringify(options)) : hash}}});
  vm.runInContext(html.match(/<script>([\s\S]*?)<\/script>/)[1], context);
  return {context, nodes, document};
}
test('ready sends battery-friendly defaults and all settings keys', () => {
  const b = bridge(); b.handlers.ready();
  assert.deepEqual(b.messages[0].settings, {'7':0,'14':0,'15':0,'16':0,'17':1,'18':0});
});
test('malformed, cancelled and non-object responses do not save or transmit', () => {
  for (const response of [undefined, '', 'CANCELLED', '%', '{', 'null', '[]', '12', '"text"', '{"display_mode":"__proto__","show_status":"true"}']) {
    const b = bridge(); b.handlers.webviewclosed({response});
    assert.equal(b.messages.length, 0); assert.deepEqual(b.stored(), {});
  }
});
test('partial encoded saves preserve previous valid settings and ignore unknown keys', () => {
  const b = bridge('{"date_format":"mon_d_aug","display_mode":"combined","analogue_seconds":true,"theme":"ice","show_status":false,"music_enabled":true}');
  b.handlers.webviewclosed({response: encodeURIComponent('{"theme":"amber","unknown":99,"show_status":"yes"}')});
  assert.deepEqual(b.messages[0].settings, {'7':2,'14':1,'15':2,'16':1,'17':0,'18':1});
  assert.deepEqual(b.stored(), {date_format:'mon_d_aug',display_mode:'combined',theme:'amber',analogue_seconds:true,show_status:false,music_enabled:true});
});
test('invalid stored settings fall back safely', () => {
  for (const stored of ['null', '[]', '%', '{"date_format":"toString","theme":99}']) {
    const b = bridge(stored); b.handlers.ready();
    assert.deepEqual(b.messages[0].settings, {'7':0,'14':0,'15':0,'16':0,'17':1,'18':0});
  }
});
test('configuration opens the standalone versioned page with migrated options', () => {
  const b = bridge('{"display_mode":"analogue"}'); b.handlers.showConfiguration();
  const url = new URL(b.urls[0]);
  assert.equal(url.origin + url.pathname, 'https://gloriouscode.github.io/pebbleeye/resources/configure-pebbleeye.html');
  assert.equal(url.searchParams.get('v'), '1.6.2');
  assert.deepEqual(JSON.parse(decodeURIComponent(url.hash.slice(9))), {display_mode:'analogue'});
});
test('failed delivery stops after three attempts', () => {
  const b = bridge(); b.handlers.ready();
  for (let attempt = 0; attempt < 3; attempt++) {
    b.messages[attempt].failure();
    if (attempt < 2) {b.timers.shift()();}
  }
  assert.equal(b.messages.length, 3); assert.equal(b.timers.length, 0);
});
test('new saves coalesce while a message is in flight and supersede retries', () => {
  const b = bridge(); b.handlers.ready();
  b.handlers.webviewclosed({response:'{"theme":"amber"}'});
  b.handlers.webviewclosed({response:'{"theme":"ice"}'});
  assert.equal(b.messages.length, 1);
  b.messages[0].failure();
  assert.equal(b.messages.length, 2); assert.equal(b.messages[1].settings['16'], 2);
  b.messages[1].failure();
  b.handlers.webviewclosed({response:'{"theme":"classic"}'});
  assert.equal(b.messages[2].settings['16'], 0);
  b.timers.shift()(); assert.equal(b.messages.length, 3);
  b.messages[2].success(); assert.equal(b.messages.length, 3);
});
test('storage failures and synchronous send failure do not crash configuration', () => {
  const b = bridge();
  b.context.localStorage.getItem = () => {throw Error('unavailable');};
  b.context.localStorage.setItem = () => {throw Error('full');};
  b.context.Pebble.sendAppMessage = () => {throw Error('offline');};
  assert.doesNotThrow(() => b.handlers.webviewclosed({response:'{"theme":"amber"}'}));
  assert.equal(b.timers.length, 1);
  b.timers.shift()(); b.timers.shift()(); assert.equal(b.timers.length, 0);
});
test('every setting combination round-trips through the form and bridge', () => {
  for (const display_mode of ['digital','analogue','combined']) {
    for (const theme of ['classic','amber','ice','orchid','lagoon','ember','silver','rose']) {
      for (const date_format of ['dd_mm_yy','mm_dd_yyyy','mon_d_aug','dd_slash_mm','mm_slash_dd']) {
        for (const analogue_seconds of [false,true]) {
          for (const show_status of [false,true]) {
            for (const music_enabled of [false,true]) {
              const options = {date_format,display_mode,theme,analogue_seconds,show_status,music_enabled};
              const e = editor(options);
              assert.deepEqual(plain(e.context.readOptions()), options);
              e.nodes.config.handlers.submit({preventDefault(){}});
              const b = bridge(); b.handlers.webviewclosed({response:e.document.location.split('#')[1]});
              assert.deepEqual(b.stored(), options);
              assert.equal(e.nodes.analogue_seconds.disabled, display_mode === 'digital');
              assert.equal(b.messages[0].settings['18'], music_enabled ? 1 : 0);
            }
          }
        }
      }
    }
  }
});
test('malformed option URLs fall back and cancel never sends options', () => {
  for (const hash of ['#options=%', '#options=null', '#options=%5B%5D', '#options=%7B%22theme%22%3A%22toString%22%7D']) {
    const e = editor({}, hash);
    assert.equal(e.context.readOptions().theme, 'classic');
    e.nodes.cancel.handlers.click(); assert.equal(e.document.location, 'pebblejs://close#');
  }
});
test('preview and undo reflect edits while preserving a hidden seconds preference', () => {
  const initial = {date_format:'mon_d_aug',display_mode:'combined',theme:'amber',analogue_seconds:true,show_status:false,music_enabled:true};
  const e = editor(initial);
  e.nodes.theme.value = 'ice'; e.nodes.display_mode.value = 'digital'; e.nodes.config.handlers.change();
  assert.equal(e.nodes['preview-ink'].attributes.fill, '#aaffff');
  assert.equal(e.nodes['preview-hands'].style.display, 'none');
  assert.equal(e.nodes['preview-time-backing'].style.display, 'none');
  assert.equal(e.nodes['edit-status'].textContent, 'Unsaved changes');
  assert.equal(e.nodes.analogue_seconds.checked, true);
  assert.equal(e.nodes.music_enabled.checked, true);
  e.nodes.music_enabled.checked = false; e.nodes.config.handlers.change();
  e.nodes.undo.handlers.click(); assert.deepEqual(plain(e.context.readOptions()), initial);
  assert.equal(e.nodes.undo.disabled, true); assert.equal(e.nodes['preview-status'].style.display, 'none');
  assert.equal(e.nodes['preview-time-backing'].style.display, '');
  assert.equal(e.nodes['preview-time'].attributes.y, '127');
});

test('all eight palettes have readable main text and matching previews', () => {
  const expected = {
    classic:['#aaffaa','#00aa00','#005500'], amber:['#ffffaa','#ffff55','#555500'],
    ice:['#aaffff','#00aaff','#000055'], orchid:['#ffaaff','#aa55ff','#550055'],
    lagoon:['#aaffff','#55aaaa','#005555'], ember:['#ffaa55','#ff5555','#550000'],
    silver:['#ffffff','#aaaaaa','#555555'], rose:['#ffaaaa','#ff5555','#550000']
  };
  function luminance(hex) {
    const channels = hex.slice(1).match(/../g).map(value => parseInt(value,16)/255)
      .map(value => value <= 0.04045 ? value/12.92 : Math.pow((value+0.055)/1.055,2.4));
    return channels[0]*0.2126+channels[1]*0.7152+channels[2]*0.0722;
  }
  Object.entries(expected).forEach(([theme, [bright, muted, panel]], index) => {
    const e = editor({theme});
    assert.deepEqual(plain(e.context.paletteColours[theme]), {bright,muted,panel});
    assert.equal(e.nodes['preview-ink'].attributes.fill, bright);
    assert.equal(e.nodes['preview-arcs'].attributes.stroke, muted);
    assert.equal(e.nodes['preview-panel'].attributes.fill, panel);
    for (const background of [panel,'#000000']) {
      const contrast = (luminance(bright)+0.05)/(luminance(background)+0.05);
      assert.ok(contrast > 4.5, theme + ' contrast ' + contrast);
    }
    const b = bridge(); b.handlers.webviewclosed({response: JSON.stringify({theme})});
    assert.equal(b.messages[0].settings['16'], index);
  });
});

test('watch renderer palettes match the page and bridge theme IDs', () => {
  const source = fs.readFileSync(path.join(root, 'src/spy_face.c'), 'utf8');
  const paletteFunction = source.match(/static SpyPalette spy_palette\(int theme\)\s*\{([\s\S]*?)\n\}/);
  assert.ok(paletteFunction, 'watch palette function is available');
  const watchPalettes = new Map();
  for (const match of paletteFunction[1].matchAll(/(default|case\s+(\d+)):\s*return\s*\(SpyPalette\)\s*\{([^}]+)\}/g)) {
    const id = match[1] === 'default' ? 0 : Number(match[2]);
    assert.ok(!watchPalettes.has(id), 'unique watch theme ID ' + id);
    const colours = [...match[3].matchAll(/spy_colour\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)/g)].map(colour => {
      const components = colour.slice(1).map(Number);
      components.forEach(component => assert.ok([0,85,170,255].includes(component), 'native Pebble colour component'));
      return '#' + components.map(component => component.toString(16).padStart(2, '0')).join('');
    });
    assert.equal(colours.length, 3, 'bright, muted and panel colours for theme ' + id);
    watchPalettes.set(id, {bright:colours[0], muted:colours[1], panel:colours[2]});
  }
  const b = bridge();
  const e = editor({});
  const expectedIDs = {classic:0, amber:1, ice:2, orchid:3, lagoon:4, ember:5, silver:6, rose:7};
  assert.deepEqual(plain(b.context.themes), expectedIDs);
  assert.deepEqual(Object.keys(e.context.paletteColours).sort(), Object.keys(expectedIDs).sort());
  assert.equal(watchPalettes.size, Object.keys(expectedIDs).length);
  for (const [theme, id] of Object.entries(expectedIDs)) {
    assert.deepEqual(watchPalettes.get(id), plain(e.context.paletteColours[theme]), theme + ' watch/page parity');
    assert.equal(b.context.prepareConfiguration(JSON.stringify({theme}))['16'], id, theme + ' bridge ID');
  }
});

test('music is opt-in and malformed values cannot enable or reset it', () => {
  const defaultEditor = editor({});
  assert.equal(defaultEditor.nodes.music_enabled.checked, false);
  for (const invalid of ['true',1,null,{},[]]) {
    const response = JSON.stringify({music_enabled:invalid});
    const fresh = bridge(); fresh.handlers.webviewclosed({response});
    assert.equal(fresh.messages.length, 0);
    const enabled = bridge('{"music_enabled":true}');
    enabled.handlers.webviewclosed({response:JSON.stringify({theme:'rose',music_enabled:invalid})});
    assert.equal(enabled.stored().music_enabled, true);
    assert.equal(enabled.messages[0].settings['18'], 1);
    const e = editor({music_enabled:invalid});
    assert.equal(e.nodes.music_enabled.checked, false);
  }
  const b = bridge('{"music_enabled":true}');
  b.handlers.webviewclosed({response:'{"music_enabled":false}'});
  assert.equal(b.messages[0].settings['18'], 0);
  assert.equal(b.stored().music_enabled, false);
});
