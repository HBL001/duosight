#SSH Troubleshooter

If you rebuild your Image and reflash the board, you will find that you can nolonger SSH or SCP to it.

```
root@6faf1febd42d:/workspace/build/mlx90640-reader# scp mlx90640-reader root@192.168.137.10:readery
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@    WARNING: REMOTE HOST IDENTIFICATION HAS CHANGED!     @
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
IT IS POSSIBLE THAT SOMEONE IS DOING SOMETHING NASTY!
Someone could be eavesdropping on you right now (man-in-the-middle attack)!
It is also possible that a host key has just been changed.
The fingerprint for the RSA key sent by the remote host is
SHA256:9wfnjkfYHhy6jaTJCr3zbwOGSSzUZ67whIJDOvsPGow.
Please contact your system administrator.

```

## What do I do ?

For me I am constantly forgetting to write this line

```
root@6faf1febd42d:/workspace/build/mlx90640-reader# ssh-keygen -f "/root/.ssh/known_hosts" -R "192.168.137.10"
# Host 192.168.137.10 found: line 4
/root/.ssh/known_hosts updated.
Original contents retained as /root/.ssh/known_hosts.old
root@6faf1febd42d:/workspace/build/mlx90640-reader# 

```
