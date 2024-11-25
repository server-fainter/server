document.addEventListener('DOMContentLoaded', () => {
    fetch('/data.json')
        .then(response => {
            if (!response.ok) {
                throw new Error('네트워크 응답에 문제가 있습니다.');
            }
            return response.json();
        })
        .then(data => {
            displayUsers(data.users);
        })
        .catch(error => {
            console.error('데이터를 가져오는 중 오류가 발생했습니다:', error);
            const userList = document.getElementById('user-list');
            userList.innerHTML = '<p>사용자 데이터를 불러오는 데 실패했습니다.</p>';
        });
});

function displayUsers(users) {
    const userList = document.getElementById('user-list');
    userList.innerHTML = ''; // 기존 내용을 초기화

    users.forEach(user => {
        const userDiv = document.createElement('div');
        userDiv.classList.add('user');

        const name = document.createElement('div');
        name.classList.add('user-name');
        name.textContent = user.name;

        const email = document.createElement('div');
        email.classList.add('user-email');
        email.textContent = user.email;

        userDiv.appendChild(name);
        userDiv.appendChild(email);
        userList.appendChild(userDiv);
    });
}
