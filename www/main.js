document.addEventListener('DOMContentLoaded', initForm);

function initForm() {
    const form = document.getElementById('postForm');
    if (form) {
        form.addEventListener('submit', handlePostRequest);
    }
}

async function handlePostRequest(event) {
    event.preventDefault();
    const form = event.target;
    const log = document.getElementById('responseLog');
    const fileInput = document.getElementById('fileData');
    
    log.textContent = "Sending request to " + form.action + "...";

    // Create empty FormData payload
    const formData = new FormData();
    
    // Append each selected file explicitly into the payload loop
    if (fileInput && fileInput.files.length > 0) {
        for (let i = 0; i < fileInput.files.length; i++) {
            formData.append('fileData[]', fileInput.files[i]);
        }
    }

    try {
        const response = await fetch(form.action, {
            method: 'POST',
            body: formData
        });
        
        displayResult(response, log);
    } catch (error) {
        log.textContent = "Connection Error:\n" + error.message;
    }
}

function displayResult(response, logElement) {
    let output = "Status: " + response.status + "\n";
    output += "Status Text: " + response.statusText + "\n\n";
    output += "Headers:\n";
    
    response.headers.forEach((value, key) => {
        output += key + ": " + value + "\n";
    });
    
    logElement.textContent = output;
}