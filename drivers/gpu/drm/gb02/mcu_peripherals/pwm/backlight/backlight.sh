echo 100 > /sys/class/backlight/gb02-pwm-backlight/brightness # 屏幕亮度调到100
cat  /sys/class/backlight/gb02-pwm-backlight/max_brightness # 查看屏幕亮度最大值

 

编写一个调整屏幕亮度的脚本，比如：screen-light.sh
echo 55 > /sys/class/backlight/gb02-pwm-backlight/brightness