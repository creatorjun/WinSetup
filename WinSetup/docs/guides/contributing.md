<!-- docs/guides/contributing.md -->

# 기여 가이드 (Contributing Guide)

## 개요

WinSetup 프로젝트에 기여해 주셔서 감사합니다. 이 문서는 코드 기여 시 준수해야 할 규칙과 절차를 설명합니다.

## 코드 스타일

### C++ 코딩 규칙

#### 네이밍 규칙

```cpp
class MyClass {};
struct MyStruct {};
enum class MyEnum { Value1, Value2 };

interface IMyInterface {
    virtual void MyMethod() = 0;
};

namespace my_namespace {
    void MyFunction();
    constexpr int kMyConstant = 42;
    int gMyGlobal = 0;
}

void MyFunction(int parameterName) {
    int localVariable = 0;
    const int kLocalConstant = 42;
}

class MyClass {
private:
    int memberVariable_;
    static int staticMember_;
};
```

#### 파일 구조

```cpp
#pragma once

#include <standard_library>
#include <abstractions/...>
#include <domain/...>

namespace winsetup {

class MyClass {
public:
    MyClass();
    ~MyClass();
    
    void PublicMethod();
    
private:
    void PrivateMethod();
    
    int memberVariable_;
};

}
```

#### 헤더 가드

```cpp
#pragma once
```

모든 헤더 파일에 `#pragma once` 사용을 권장합니다.

### 포매팅 규칙

#### 들여쓰기
- **4 스페이스** 사용 (탭 사용 금지)

#### 중괄호 스타일

```cpp
void MyFunction() {
    if (condition) {
        DoSomething();
    } else {
        DoSomethingElse();
    }
}

class MyClass {
public:
    MyClass() : member_(0) {
    }
};
```

#### 한 줄 길이
- 최대 **100자**로 제한

#### 공백

```cpp
int x = 10;

if (condition) {
    DoSomething();
}

for (int i = 0; i < 10; ++i) {
    Process(i);
}

auto result = MyFunction(arg1, arg2, arg3);
```

## 의존성 규칙 준수

### 계층 간 의존성 검증

```cpp
#include <abstractions/io/IFileSystem.h>
#include <domain/primitives/Result.h>

class FileManager {
    IFileSystem& fileSystem;
    
public:
    Result<void> CreateProjectDirectory();
};
```

### 금지 사항

```cpp
#include <adapters/platform/windows/Win32FileSystem.h>
class MyService {
    Win32FileSystem fileSystem;
};
```

## 커밋 메시지 규칙

### 커밋 메시지 형식

```
<type>(<scope>): <subject>

<body>

<footer>
```

### Type
- **feat**: 새로운 기능
- **fix**: 버그 수정
- **docs**: 문서 변경
- **style**: 코드 포매팅
- **refactor**: 리팩토링
- **test**: 테스트 추가/수정
- **chore**: 빌드 프로세스 변경

### 예제

```
feat(domain): Expected<T> 모나드 구현

- Map, FlatMap 메서드 추가
- Error 타입 통합
- 단위 테스트 작성

Closes #42
```

```
fix(adapters): Win32FileSystem 메모리 누수 수정

CreateFileW 호출 후 핸들이 제대로 닫히지 않던 문제 해결

Fixes #156
```

## Pull Request 프로세스

### 1. Fork & Clone

```bash
git clone https://github.com/your-username/WinSetup.git
cd WinSetup
```

### 2. 브랜치 생성

```bash
git checkout -b feature/my-new-feature
```

브랜치 네이밍 규칙:
- `feature/` - 새로운 기능
- `fix/` - 버그 수정
- `docs/` - 문서 변경
- `refactor/` - 리팩토링

### 3. 코드 작성

- 코딩 규칙 준수
- 의존성 규칙 준수
- 단위 테스트 작성

### 4. 테스트 실행

```cmd
scripts\build.bat
scripts\test.bat
```

모든 테스트가 통과해야 합니다.

### 5. 커밋

```bash
git add .
git commit -m "feat(domain): 새로운 기능 추가"
```

### 6. Push

```bash
git push origin feature/my-new-feature
```

### 7. Pull Request 생성

GitHub에서 Pull Request를 생성하고 다음 정보를 포함합니다:
- 변경 사항 설명
- 관련 이슈 번호
- 테스트 결과
- 스크린샷 (UI 변경 시)

## 코드 리뷰

### 리뷰어 체크리스트

- [ ] 의존성 규칙 준수
- [ ] 코딩 스타일 준수
- [ ] 단위 테스트 존재
- [ ] 테스트 통과
- [ ] 문서 업데이트 (필요 시)
- [ ] 커밋 메시지 규칙 준수

### 리뷰 프로세스

1. 최소 1명의 승인 필요
2. 모든 코멘트 해결
3. CI 테스트 통과
4. Squash and Merge

## 이슈 보고

### 버그 리포트

```markdown
## 버그 설명
간단한 버그 설명

## 재현 방법
1. 단계 1
2. 단계 2
3. 단계 3

## 예상 동작
무엇이 일어나야 하는지

## 실제 동작
실제로 무엇이 일어났는지

## 환경
- OS: Windows 10 21H2
- 컴파일러: MSVC 2022
- 버전: v1.0.0

## 추가 정보
스크린샷, 로그 등
```

### 기능 요청

```markdown
## 기능 설명
새로운 기능에 대한 설명

## 동기
왜 이 기능이 필요한지

## 대안
고려한 다른 대안들

## 추가 정보
기타 컨텍스트
```

## 개발 환경 설정

### 필수 도구

- Visual Studio 2022
- Windows SDK 10.0.22621.0 이상
- Git

### 프로젝트 빌드

```cmd
git clone https://github.com/your-org/WinSetup.git
cd WinSetup
scripts\build.bat
```

### 테스트 실행

```cmd
scripts\test.bat
```

## 라이선스

모든 기여는 프로젝트 라이선스를 따릅니다.

## 행동 강령

- 존중과 포용
- 건설적인 피드백
- 협력적 태도
- 전문성 유지

## 연락처

질문이나 도움이 필요하면:
- GitHub Issues
- Email: dev@winsetup.com

## 정리

기여 규칙을 준수하여 고품질의 코드를 유지합니다.
