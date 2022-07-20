cmd_/home/andy/codes/chardev/modules.order := {   echo /home/andy/codes/chardev/chardev.ko; :; } | awk '!x[$$0]++' - > /home/andy/codes/chardev/modules.order
