## 요구 사항

다음 패키지가 설치되어 있어야 합니다:

```bash
sudo apt install libwebsockets-dev pkg-config libcjson-dev
```


## 동작 흐름
1. 서버 초기화
서버가 클라이언트 요청을 처리하기 위해 초기화됩니다.

2. 클라이언트 접속
클라이언트가 웹 브라우저를 통해 서버에 접속합니다.
예: http://localhost:8080

3. 정적 파일 요청 및 전송
클라이언트는 서버에 HTML, CSS, JavaScript 등의 정적 파일을 요청합니다.
서버는 해당 파일을 클라이언트로 전송하여 초기 화면을 렌더링합니다.

4. WebSocket 연결
클라이언트가 서버에 WebSocket 연결을 요청합니다.
이후 실시간으로 픽셀 데이터가 서버와 클라이언트 간에 교환됩니다.


## 파일 구조
project/
├── static/
│   ├── index.html     # 클라이언트 HTML 파일
│   ├── script.js      # 클라이언트 JavaScript 파일
│   └── styles.css     # 클라이언트 CSS 파일
├── Makefile           # 프로젝트 빌드 파일
├── README.md          # 프로젝트 설명 파일
└── server.c           # 서버 구현 코드


## 실행 방법
1. 서버 실행
서버 소스 코드를 컴파일하고 실행합니다.
예: ./server

2. 클라이언트 접속
브라우저를 열고 http://localhost:8080에 접속합니다.

3. 픽셀 그리기
클라이언트는 화면의 픽셀을 클릭하여 색상을 변경할 수 있습니다.
변경된 픽셀 정보는 실시간으로 서버와 동기화됩니다.