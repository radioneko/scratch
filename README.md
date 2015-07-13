This is my personal playground to test miscellaneous stuff
and know more about how some system functions are working.

Some private data is hidden in ignored config.h, so this
may be not very useful to most peope.

### epoll.c

Investigating how epoll works in edge-triggered mode.
* It seems that event is fired every time descriptor state changed.
So you actually _not always_ need to check to read/write until
`EAGAIN` (if you have read/wrote all you needed, just go into
epoll_wait again, possibly marking descriptor as
"ready-for-read/ready-for-write" just like nginx does)
* `EPOLLRDHUP` appears on sockets as a result of receiving FIN
message (via shutdown). To get `EPOLLRDHUP`, you'll need explicitly
set it in events mask.
  
### transfer.c

Attempt to investigate how effective `EPOLLET` + `splice(2)` can be.

### sha.c

If reading/restoring checksum state works as expected (for resuming of
large transfers checksums).

### lib-*.c

Miscellaneous wrappers to make writing test samples easy-breezy
without tons of boilerplate crap.
