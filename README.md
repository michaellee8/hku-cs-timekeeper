# hku-cs-timekeeper
An attempt to implement timekeeper project in COMP3230A
Provides functionalties like `time` command does, but with piping `|` and more running statistics

System requirement:
Any POSIX-compliant system with `wait4` system call and `gcc` installed
Run me with this 
```bash
curl https://raw.githubusercontent.com/michaellee8/hku-cs-timekeeper/master/main.c -o hkucstimekeeper.c && \
gcc hkucstimekeeper.c -o hkucstimekeeper && \
sudo mv ./hkucstimekeeper /usr/local/bin/hkucstimekeeper && \
hkucstimekeeper curl https://raw.githubusercontent.com/michaellee8/hku-cs-timekeeper/master/main.c | grep include | cat -n | cat -n
```
