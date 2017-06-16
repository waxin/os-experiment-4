echo "start complie.."
gcc attack.c -o attack -lpthread
echo "start excuting.."
./attack
echo "show final--"
ls /success -ahl

