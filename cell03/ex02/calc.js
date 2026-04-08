// 1. Every 30 seconds, show the nag message
setInterval(function() {
    alert('Please, use me...');
}, 30000);

document.getElementById('submitBtn').addEventListener('click', function() {
    // Get values from the DOM
    const leftValRaw = document.getElementById('leftMember').value;
    const rightValRaw = document.getElementById('rightMember').value;
    const operator = document.getElementById('operator').value;

    // Convert to numbers
    const leftNum = Number(leftValRaw);
    const rightNum = Number(rightValRaw);

    // Validation: Check if positive integers
    // We check if they are integers, >= 0, and not empty strings
    if (leftValRaw === "" || rightValRaw === "" || 
        !Number.isInteger(leftNum) || !Number.isInteger(rightNum) || 
        leftNum < 0 || rightNum < 0) {
        alert('Error :(');
        return;
    }

    // Check for division/modulo by zero
    if ((operator === '/' || operator === '%') && rightNum === 0) {
        const msg = "It’s over 9000!";
        console.log(msg);
        alert(msg);
        return;
    }

    // Execute Calculation
    let result;
    switch (operator) {
        case '+': result = leftNum + rightNum; break;
        case '-': result = leftNum - rightNum; break;
        case '*': result = leftNum * rightNum; break;
        case '/': result = leftNum / rightNum; break;
        case '%': result = leftNum % rightNum; break;
    }

    // Display Result
    console.log(result);
    alert(result);
});