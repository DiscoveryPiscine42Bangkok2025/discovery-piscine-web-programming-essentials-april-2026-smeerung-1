const ftList = document.getElementById('ft_list');
const newBtn = document.getElementById('new_btn');

// Load list from cookies on startup
window.onload = () => {
    const cookies = document.cookie.split('; ');
    const todoCookie = cookies.find(row => row.startsWith('todos='));
    if (todoCookie) {
        const tasks = JSON.parse(decodeURIComponent(todoCookie.split('=')[1]));
        tasks.reverse().forEach(task => addTask(task, false));
    }
};

// Save list to cookies
function saveToCookie() {
    const tasks = Array.from(ftList.children).map(div => div.textContent);
    document.cookie = `todos=${encodeURIComponent(JSON.stringify(tasks))}; path=/; max-age=31536000`;
}

// Add task function
function addTask(text, save = true) {
    if (!text) return;
    
    const div = document.createElement('div');
    div.textContent = text;
    
    // Remove on click
    div.onclick = () => {
        if (confirm("Do you want to remove this TO DO?")) {
            div.remove();
            saveToCookie();
        }
    };

    ftList.prepend(div);
    if (save) saveToCookie();
}

// Button event
newBtn.onclick = () => {
    const task = prompt("Enter a new TO DO:");
    if (task && task.trim() !== "") {
        addTask(task.trim());
    }
};
