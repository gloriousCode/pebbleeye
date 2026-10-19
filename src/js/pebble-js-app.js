var VERSION = "1.6.2";
var OPTIONS_STORAGE_KEY = "gabbroeye-hud-options-v1";
var CONFIG_URL = "https://gloriouscode.github.io/pebbleeye/resources/configure-pebbleeye.html";
var dateFormats = {dd_mm_yy: 0, mm_dd_yyyy: 1, mon_d_aug: 2, dd_slash_mm: 3, mm_slash_dd: 4};
var displayModes = {digital: 0, analogue: 1, combined: 2};
var themes = {classic: 0, amber: 1, ice: 2, orchid: 3, lagoon: 4, ember: 5, silver: 6, rose: 7};
var pendingConfiguration = null;
var deliveringConfiguration = false;
var configurationGeneration = 0;

function parseConfigurationResponse(response) {
  if (typeof response !== "string" || !response || response === "CANCELLED") { return {}; }
  var value;
  try { value = JSON.parse(response); }
  catch (error) {
    try { value = JSON.parse(decodeURIComponent(response)); }
    catch (decodeError) { return {}; }
  }
  return value && typeof value === "object" && !Array.isArray(value) ? value : {};
}

function validOptions(value) {
  var result = {};
  var enums = {date_format: dateFormats, display_mode: displayModes, theme: themes};
  Object.keys(enums).forEach(function(key) {
    if (typeof value[key] === "string" && Object.prototype.hasOwnProperty.call(enums[key], value[key])) {
      result[key] = value[key];
    }
  });
  ["analogue_seconds", "show_status"].forEach(function(key) {
    if (typeof value[key] === "boolean") { result[key] = value[key]; }
  });
  return result;
}

function getOptions() {
  var stored = "{}";
  try { stored = localStorage.getItem(OPTIONS_STORAGE_KEY) || "{}"; }
  catch (error) { console.log("Unable to read PebbleEye settings."); }
  return JSON.stringify(validOptions(parseConfigurationResponse(stored)));
}

function prepareConfiguration(serialized) {
  var value = validOptions(parseConfigurationResponse(serialized));
  return {"7": dateFormats[value.date_format] || 0,
    "14": value.analogue_seconds === true ? 1 : 0,
    "15": displayModes[value.display_mode] || 0,
    "16": themes[value.theme] || 0,
    "17": value.show_status === false ? 0 : 1};
}

// One delivery at a time: a newer save supersedes queued retries of older settings.
function transmitConfiguration(settings) {
  pendingConfiguration = {settings: settings, generation: ++configurationGeneration, attempts: 0};
  deliverPendingConfiguration();
}

function deliverPendingConfiguration() {
  if (deliveringConfiguration || !pendingConfiguration) { return; }
  var job = pendingConfiguration;
  pendingConfiguration = null;
  deliveringConfiguration = true;
  job.attempts++;
  function complete(success) {
    deliveringConfiguration = false;
    if (!success && job.generation === configurationGeneration && job.attempts < 3) {
      pendingConfiguration = job;
      setTimeout(deliverPendingConfiguration, 1000 * job.attempts);
    } else {
      if (!success && job.generation === configurationGeneration) {
        console.log("PebbleEye settings could not be delivered; they will retry when the app reconnects.");
      }
      deliverPendingConfiguration();
    }
  }
  try {
    Pebble.sendAppMessage(job.settings, function() { complete(true); }, function() { complete(false); });
  } catch (error) { complete(false); }
}

function webviewclosed(event) {
  var changes = validOptions(parseConfigurationResponse(event && event.response));
  if (!Object.keys(changes).length) { return; }
  var options = parseConfigurationResponse(getOptions());
  Object.keys(changes).forEach(function(key) { options[key] = changes[key]; });
  var serialized = JSON.stringify(options);
  try { localStorage.setItem(OPTIONS_STORAGE_KEY, serialized); }
  catch (error) { console.log("Unable to store PebbleEye settings."); }
  transmitConfiguration(prepareConfiguration(serialized));
}

Pebble.addEventListener("ready", function() { transmitConfiguration(prepareConfiguration(getOptions())); });
Pebble.addEventListener("showConfiguration", function() {
  Pebble.openURL(CONFIG_URL + "?v=" + encodeURIComponent(VERSION) + "#options=" + encodeURIComponent(getOptions()));
});
Pebble.addEventListener("webviewclosed", webviewclosed);
