@echo off
sc pause "Courseware Thief Service"
ping -n 1 127.0.0.1
net stop "Courseware Thief Service"
exit 0
