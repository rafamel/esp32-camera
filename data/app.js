import htm from './lib/htm.module.js';
import { h, render } from './lib/preact.module.js';

const html = htm.bind(h);

Promise.all([
  fetch('./api/params').then((res) => res.json()),
  fetch('./cameras.json').then((res) => res.json())
]).then(([params, cameras]) => {
  setTimeout(() => {
    const serverPort = document.location.port;
    const serverUrl = document.location.origin;
    const baseUrl = serverPort
      ? serverUrl.slice(0, -(serverPort.length + 1))
      : serverUrl;
    const streamUrl = params.streamPort == 80
      ? serverUrl
      : baseUrl.replace(/\/$/, '') + ':' + String(params.streamPort);

    start({ serverUrl, streamUrl });
  }, 0);

  render(
    html`<${App} params=${params} cameras=${cameras} />`,
    document.body
  );
});

function App({ params, cameras }) {
  return html`
    <section class="main">
      <div id="logo">
        <label for="nav-toggle-cb" id="nav-toggle">
          ☰ Settings
        </label>
      </div>
      <div id="content">
        <div id="sidebar">
          <input type="checkbox" id="nav-toggle-cb" checked="checked" />
          <${Menu} params=${params} cameras=${cameras} />
        </div>
        <figure>
          <div id="stream-container" class="image-container hidden">
            <div class="close" id="close-stream">×</div>
            <img id="stream" src="" />
          </div>
        </figure>
      </div>
    </section>
  `;
}

function Menu({ params, cameras }) {
  const cameraModel = params.cameraModel;
  const settings = Object.entries(cameras)
    .filter((entry) => entry[1].models.includes(cameraModel))
    .map(([key, item]) => ({
      id: key,
      type: item.type,
      name: item.name,
      values: item.values 
        && Object.hasOwnProperty.call(item.values, cameraModel) 
          ? item.values[cameraModel]
          : item.defaults
    }))
    .map((item) => {
      if (item.type === 'select') return html`<${Select} ...${item} />`;
      if (item.type === 'range') return html`<${Range} ...${item} />`;
      if (item.type === 'switch') return html`<${Switch} ...${item} />`;
      return null;
    })
    .filter(item => Boolean(item));

  return html`
    <nav id="menu">
      ${settings}
      <section id="buttons">
        <button id="get-still">Get Still</button>
        <button id="toggle-stream">Start Stream</button>
      </section>
    </nav>
  `;
}

function Select({ id, name, values }) {
  const options = Object.entries(values)
    .map((item) => html`<option value="${item[0]}">${item[1]}</option>`);

  return html`
    <div class="input-group" id="${id}-group">
      <label for="${id}">${name}</label>
      <select id="${id}" class="default-action">
        ${options}
      </select>
    </div>
  `;
}

function Range({ id, name, values: { min, max } }) {
  return html`
    <div class="input-group" id="${id}-group">
      <label for="${id}">${name}</label>
      <div class="range-min">${min}</div>
      <input
        type="range"
        id="${id}"
        min="${min}"
        max="${max}"
        class="default-action"
      />
      <div class="range-max">${max}</div>
    </div>
  `;
}

function Switch({ id, name }) {
  return html`
    <div class="input-group" id="${id}-group">
      <label for="${id}">${name}</label>
      <div class="switch">
        <input
          id="${id}"
          type="checkbox"
          class="default-action"
          checked="checked"
        />
        <label class="slider" for="${id}"></label>
      </div>
    </div>
  `;
}

function start({ serverUrl, streamUrl }) {

  let queue = Promise.resolve();
  const hide = (el) => el ? el.classList.add('hidden') : undefined;
  const show = (el) => el ? el.classList.remove('hidden') : undefined;

  function updateConfig(el) {
    let value;
    switch (el.type) {
      case 'checkbox':
        value = el.checked ? 1 : 0;
        break;
      case 'range':
      case 'select-one':
        value = el.value;
        break;
      case 'button':
      case 'submit':
        value = '1';
        break;
      default:
        return;
    }

    queue = queue
      .then(() => {
        return fetch(
          `${serverUrl}/api/control?property=${el.id}&value=${value}`
        );
      })
      .then((res) => {
        console.log(
          `Finished request to control endpoint, status: ${res.status}`
        );
      })
      .catch((err) => {
        console.error(err);
        hydrateElements();
        window.alert('Failed to apply settings');
      });
  }

  function updateValue(el, value, updateRemote) {
    updateRemote = updateRemote == null ? true : updateRemote;
    let initialValue;
    if (el.type === 'checkbox') {
      initialValue = el.checked;
      value = !!value;
      el.checked = value;
    } else {
      initialValue = el.value;
      el.value = value;
    }

    if (updateRemote && initialValue !== value) {
      updateConfig(el);
    } else if (!updateRemote) {
      if (el.id === 'aec') {
        value ? hide(exposure) : show(exposure);
      } else if (el.id === 'agc') {
        if (value) {
          show(gainCeiling);
          hide(agcGain);
        } else {
          hide(gainCeiling);
          show(agcGain);
        }
      } else if (el.id === 'awb_gain') {
        value ? show(wb) : hide(wb);
      }
    }
  };

  function hydrateElements() {
    queue = queue
      .then(() => fetch(`${serverUrl}/api/status`))
      .then((response) => response.json())
      .then((state) => {
        document.querySelectorAll('.default-action').forEach((el) => {
          updateValue(el, state[el.id], false);
        });
      })
      .catch(err => {
        console.error(err);
        setTimeout(hydrateElements, 2500);
      })
  }

  document.querySelectorAll('.close').forEach((el) => {
    el.onclick = () => {
      hide(el.parentNode);
    };
  });

  hydrateElements();
  const view = document.getElementById('stream');
  const viewContainer = document.getElementById('stream-container');
  const stillButton = document.getElementById('get-still');
  const streamButton = document.getElementById('toggle-stream');
  const closeButton = document.getElementById('close-stream');

  const stopStream = () => {
    queue = queue.then(() => window.stop())
    streamButton.innerHTML = 'Start Stream';
  };

  const startStream = () => {
    queue = queue.then(() => {
      view.src = `${streamUrl}/stream`;
      show(viewContainer);
    });
    streamButton.innerHTML = 'Stop Stream';
  };

  // Attach actions to buttons
  stillButton.onclick = () => {
    stopStream();
    queue = queue
      .then(() => new Promise((resolve) => setTimeout(resolve, 100)))
      .then(() => {
        view.src = `${serverUrl}/capture?_cb=${Date.now()}`
        show(viewContainer);
      });
  };

  closeButton.onclick = () => {
    stopStream();
    hide(viewContainer);
  };

  streamButton.onclick = () => {
    const streamEnabled = streamButton.innerHTML === 'Stop Stream';
    if (streamEnabled) {
      stopStream();
    } else {
      startStream();
    }
  };

  // Attach default on change action
  document.querySelectorAll('.default-action').forEach((el) => {
    el.onchange = () => updateConfig(el);
  });

  // Custom actions
  // Gain
  const agc = document.getElementById('agc');
  const agcGain = document.getElementById('agc_gain-group');
  const gainCeiling = document.getElementById('gainceiling-group');
  if (agc) {
    agc.onchange = () => {
      updateConfig(agc);
      if (agc.checked) {
        show(gainCeiling);
        hide(agcGain);
      } else {
        hide(gainCeiling);
        show(agcGain);
      }
    };
  }

  // Exposure
  const aec = document.getElementById('aec');
  const exposure = document.getElementById('aec_value-group');
  if (aec) {
    aec.onchange = () => {
      updateConfig(aec);
      aec.checked ? hide(exposure) : show(exposure);
    };
  }

  // AWB
  const awb = document.getElementById('awb_gain');
  const wb = document.getElementById('wb_mode-group');
  if (awb) {
    awb.onchange = () => {
      updateConfig(awb);
      awb.checked ? show(wb) : hide(wb);
    };
  }
}
