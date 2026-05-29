import time
import datetime
import os
import holidays

def is_workday(today):
    # 한국 공휴일 설정
    kr_holidays = holidays.KR()
    # 주말(토:5, 일:6)이 아니고 공휴일이 아니면 True
    return today.weekday() < 5 and today not in kr_holidays

def show_alarm(message):
    # 새로운 CMD 창을 띄워 메시지 출력 (10초 후 자동 종료)
    os.system(f'start cmd /K "echo {message} && timeout /t 10"')

print("옥상 문 관리 알림 프로그램이 실행 중입니다...")

while True:
    now = datetime.datetime.now()
    current_time = now.strftime("%H:%M")
    
    # 평일인지 확인
    if is_workday(now.date()):
        # 오전 9시: 문 열기
        if current_time == "09:00":
            show_alarm("!!! [알림] 오전 9시입니다. 옥상 문을 열어주세요 !!!")
            time.sleep(61) # 중복 실행 방지
            
        # 오후 5시 50분: 문 닫기
        elif current_time == "17:50":
            show_alarm("!!! [알림] 오후 5시 50분입니다. 옥상 문을 닫아주세요 !!!")
            time.sleep(61) # 중복 실행 방지

    # 30초마다 체크
    time.sleep(30)