The extend() method is now called instead of the append() method when unpickle
collections.deque and other list-like objects. This can speed up unpickling to
2 times.
