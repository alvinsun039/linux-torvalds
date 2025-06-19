rm ./test_glmark2.log
rm ./dma_result800x600.txt
export LD_LIBRARY_PATH=/usr/local/sietium/lib64/
echo "" > /var/log/kern.log
glmark2 -f 10fps
cp /var/log/kern.log ./test_glmark2.log
python parse.py ./test_glmark2.log 800x600
