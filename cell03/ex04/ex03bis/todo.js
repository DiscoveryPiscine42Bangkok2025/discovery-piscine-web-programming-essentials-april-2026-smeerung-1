document.addEventListener("DOMContentLoaded", loadList);


function newElement() {
    let inputValue = document.getElementById("myInput").value;
    if (inputValue.trim() === '') {
        alert("You must write something!");
        return;
    }

    createListItem(inputValue, false);
    document.getElementById("myInput").value = "";
    saveList();
}


function createListItem(text, isChecked) {
    const li = document.createElement("li");
    li.textContent = text;
    if (isChecked) li.classList.add("checked");

   
    li.addEventListener('click', function(e) {
        if (e.target.tagName === 'LI') {
            li.classList.toggle('checked');
            saveList();
        }
    });

    
    const span = document.createElement("SPAN");
    const txt = document.createTextNode("\u00D7");
    span.className = "close";
    span.appendChild(txt);
    
  
    span.onclick = function() {
        li.remove();
        saveList();
    };

    li.appendChild(span);
    document.getElementById("myUL").appendChild(li);
}


function saveList() {
    let items = [];
    document.querySelectorAll("#myUL li").forEach(li => {
        
        items.push({
            text: li.childNodes[0].textContent,
            checked: li.classList.contains("checked")
        });
    });
    localStorage.setItem("todoList", JSON.stringify(items));
}


function loadList() {
    let data = localStorage.getItem("todoList");
    let ul = document.getElementById("myUL");
    
   
    ul.innerHTML = "";

    if (data) {
        let items = JSON.parse(data);
        items.forEach(item => {
            createListItem(item.text, item.checked);
        });
    }
}