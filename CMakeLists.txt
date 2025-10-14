cmake_minimum_required(VERSION 3.20)
project(CSTP C)

set(CMAKE_C_STANDARD 99)

# prob1: 단독 실행
add_executable(prob1 prob1.c)

# prob2: 단독 실행 (헤더는 굳이 SOURCES에 넣을 필요 없음)
add_executable(prob2 prob2.c)

# prob3: 단일 소스만! (prob3.c 안에서 prob2.c를 #include로 텍스트 포함)
add_executable(prob3 prob3.c)