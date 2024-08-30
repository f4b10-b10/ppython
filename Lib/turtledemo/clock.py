"""       turtle-example-suite:

             tdemo_clock.py

Enhanced clock-program, showing date
and time
  ------------------------------------
   Press STOP to exit the program!
  ------------------------------------
"""
from turtle import *
from datetime import datetime

def jump(distanz, winkel=0):
    penup()
    right(winkel)
    forward(distanz)
    left(winkel)
    pendown()

def hand(laenge, spitze):
    fd(laenge*1.15)
    rt(90)
    fd(spitze/2.0)
    lt(120)
    fd(spitze)
    lt(120)
    fd(spitze)
    lt(120)
    fd(spitze/2.0)

def make_hand_shape(name, laenge, spitze):
    reset()
    jump(-laenge*0.15)
    begin_poly()
    hand(laenge, spitze)
    end_poly()
    hand_form = get_poly()
    register_shape(name, hand_form)

def clockface(radius):
    reset()
    pensize(7)
    for i in range(60):
        jump(radius)
        if i % 5 == 0:
            fd(25)
            jump(-radius-25)
        else:
            dot(3)
            jump(-radius)
        rt(6)

def display_date_time():
    writer.clear()
    now = datetime.now()
    writer.home()
    writer.forward(distance=65)
    writer.write(now.strftime(format="%A"),
                 align="center", font=("TkFixedFont", 14, "bold"))
    writer.back(distance=150)
    writer.write(now.strftime(format="%Y/%m/%d"),
                 align="center", font=("TkFixedFont", 14, "bold"))
    writer.forward(distance=85)

def initialize_hand(shape, color):
    hand = Turtle()
    hand.shape(shape)
    hand.color(*color)
    hand.resizemode("user")
    hand.shapesize(1, 1, 3)
    hand.speed(0)
    return hand

def setup():
    global second_hand, minute_hand, hour_hand, writer
    mode("logo")
    make_hand_shape("second_hand", 125, 25)
    make_hand_shape("minute_hand",  115, 25)
    make_hand_shape("hour_hand", 90, 25)
    clockface(160)

    second_hand = initialize_hand("second_hand", ("gray20", "gray80"))
    minute_hand = initialize_hand( "minute_hand", ("blue1", "red1"))
    hour_hand = initialize_hand( "hour_hand", ("blue3", "red3"))

    ht()
    writer = Turtle()
    writer.ht()
    writer.pu()
    writer.bk(85)
    display_date_time()

def tick():
    now = datetime.now()
    sekunde = now.second + now.microsecond * 0.000001
    minute = now.minute + sekunde / 60.0
    stunde = now.hour + minute / 60.0

    try:
        tracer(False)  # Terminator can occur here
        second_hand.setheading(6*sekunde)  # or here
        minute_hand.setheading(6*minute)
        hour_hand.setheading(30*stunde)
        tracer(True)
        ontimer(tick, 100)
    except Terminator:
        pass  # turtledemo user pressed STOP

def main():
    tracer(False)
    setup()
    tracer(True)
    tick()
    return "EVENTLOOP"

if __name__ == "__main__":
    mode("logo")
    msg = main()
    print(msg)
    mainloop()
