// WiFi Manager JavaScript - ESP32 CYD WiFi Setup
let selectedNetwork = null
let networks = []

function loadNetworks () {
  console.log('Starting to load networks...')
  document.getElementById('wifiList').innerHTML =
    '<div class="loading">Loading networks via JavaScript...</div>'

  fetch('/wifi')
    .then(response => {
      console.log('Received response:', response.status)
      if (!response.ok) {
        throw new Error('HTTP error! status: ' + response.status)
      }
      return response.json()
    })
    .then(data => {
      console.log('Received data:', data)
      networks = data.networks || []
      displayNetworks()
    })
    .catch(error => {
      console.error('Error loading networks:', error)
      document.getElementById('wifiList').innerHTML =
        '<div class="loading">JavaScript Error: ' +
        error.message +
        '<br>Please use the <a href="/wifi">Test WiFi Endpoint</a> link to verify data is available.</div>'
    })
}

function displayNetworks () {
  const wifiList = document.getElementById('wifiList')

  if (networks.length === 0) {
    wifiList.innerHTML =
      '<div class="loading">No networks found. Use manual entry below.</div>'
    return
  }

  let html = ''
  networks.forEach((network, index) => {
    const signalIcon = getSignalIcon(network.quality)
    const securityIcon = network.secure ? '🔒' : '🔓'

    html += `<div class="wifi-item" onclick="selectNetwork(${index})" id="network-${index}">`
    html += `<span class="wifi-name">${escapeHtml(network.ssid)}</span>`
    html += `<span class="wifi-signal">${signalIcon} ${network.quality}%</span>`
    html += `<span class="wifi-security">${securityIcon} ${network.auth}</span>`
    html += '</div>'
  })

  wifiList.innerHTML = html
}

function getSignalIcon (quality) {
  if (quality >= 80) return '📶'
  if (quality >= 60) return '📶'
  if (quality >= 40) return '📶'
  if (quality >= 20) return '📶'
  return '📶'
}

function selectNetwork (index) {
  // Remove previous selection
  if (selectedNetwork !== null) {
    const prevElement = document.getElementById('network-' + selectedNetwork)
    if (prevElement) {
      prevElement.classList.remove('selected')
    }
  }

  // Add new selection
  selectedNetwork = index
  const currentElement = document.getElementById('network-' + index)
  if (currentElement) {
    currentElement.classList.add('selected')
  }

  // Fill in the form (only if elements exist - setup page)
  const network = networks[index]
  const ssidElement = document.getElementById('ssid')
  const passwordElement = document.getElementById('password')

  if (ssidElement && passwordElement) {
    ssidElement.value = network.ssid
    passwordElement.focus()
  }
}

function escapeHtml (text) {
  const div = document.createElement('div')
  div.textContent = text
  return div.innerHTML
}

// Page initialization
window.onload = function () {
  console.log('Page loaded, starting initialization...')
  console.log('Base URL:', window.location.origin)

  // Check if this is the setup page (has wifiList element)
  const wifiListElement = document.getElementById('wifiList')
  if (wifiListElement) {
    console.log('Setup page detected - starting network loading...')
    wifiListElement.innerHTML =
      '<div class="loading">Page loaded, scanning for networks...</div>'

    // Try multiple times to catch the scan completion
    setTimeout(loadNetworks, 2000) // 2 seconds
    setTimeout(loadNetworks, 5000) // 5 seconds
    setTimeout(loadNetworks, 10000) // 10 seconds

    // Refresh network list every 30 seconds
    setInterval(loadNetworks, 30000)
  } else {
    console.log('Configuration page detected - skipping network scan')
  }

  // Load configuration parameters (available on both pages)
  const configFormElement = document.getElementById('configForm')
  if (configFormElement) {
    loadConfiguration()
  }
}

// Configuration management functions
function loadConfiguration () {
  console.log('Loading configuration...')

  fetch('/config')
    .then(response => {
      console.log('Config response:', response.status)
      if (!response.ok) {
        throw new Error('HTTP error! status: ' + response.status)
      }
      return response.json()
    })
    .then(data => {
      console.log('Received config data:', data)
      displayConfiguration(data.parameters || [])
    })
    .catch(error => {
      console.error('Error loading configuration:', error)
      const configForm = document.getElementById('configForm')
      if (configForm) {
        configForm.innerHTML =
          '<div class="error">Failed to load configuration: ' +
          error.message +
          '</div>'
      }
    })
}

function displayConfiguration (parameters) {
  const configForm = document.getElementById('configForm')

  if (!configForm) {
    console.warn('configForm element not found')
    return
  }

  if (parameters.length === 0) {
    configForm.innerHTML =
      '<div class="info">No configuration parameters defined.</div>'
    return
  }

  let html = '<div class="config-params">'

  parameters.forEach(param => {
    html += '<div class="config-param">'
    html += `<label for="config_${param.key}">${escapeHtml(
      param.label
    )}:</label>`

    if (param.type === 'checkbox') {
      const checked =
        param.value === 'true' || param.value === '1' ? 'checked' : ''
      html += `<input type="checkbox" id="config_${param.key}" name="${
        param.key
      }" ${checked} ${param.required ? 'required' : ''}>`
    } else if (param.type === 'number') {
      html += `<input type="number" id="config_${param.key}" name="${
        param.key
      }" value="${escapeHtml(param.value)}" placeholder="${escapeHtml(
        param.placeholder
      )}" ${param.required ? 'required' : ''}>`
    } else {
      const inputType = param.key.includes('password') ? 'password' : 'text'
      html += `<input type="${inputType}" id="config_${param.key}" name="${
        param.key
      }" value="${escapeHtml(param.value)}" placeholder="${escapeHtml(
        param.placeholder
      )}" ${param.required ? 'required' : ''}>`
    }

    html += '</div>'
  })

  html += '</div>'
  configForm.innerHTML = html

  // Show the save button (only if it exists - config page)
  const saveBtn = document.getElementById('saveConfigBtn')
  if (saveBtn) {
    saveBtn.style.display = 'block'
  }
}

function saveConfiguration () {
  console.log('Saving configuration...')

  const formData = new FormData()
  const configParams = document.querySelectorAll('#configForm input')

  configParams.forEach(input => {
    if (input.type === 'checkbox') {
      formData.append(input.name, input.checked ? 'true' : 'false')
    } else {
      formData.append(input.name, input.value)
    }
  })

  // Convert FormData to URL-encoded string
  const urlEncoded = new URLSearchParams(formData).toString()

  fetch('/config/save', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/x-www-form-urlencoded'
    },
    body: urlEncoded
  })
    .then(response => {
      console.log('Save response:', response.status)
      if (!response.ok) {
        throw new Error('HTTP error! status: ' + response.status)
      }
      return response.json()
    })
    .then(data => {
      console.log('Save result:', data)
      if (data.status === 'success') {
        alert('Configuration saved successfully!')
      } else {
        alert('Configuration save result: ' + data.message)
      }
    })
    .catch(error => {
      console.error('Error saving configuration:', error)
      alert('Failed to save configuration: ' + error.message)
    })
}

// Manual test function
function testFetch () {
  console.log('Manual test triggered')
  loadNetworks()
  loadConfiguration()
}
