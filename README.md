#opencv cascade classfilter 记录文档

> 该测试均在opencv-2.4.9下进行。其中HyperLPR原本需要运行opencv-3.4.0以后版本。    
> 
> 本文功能在192.168.200.96的虚拟上进行运行，运行在虚拟机主要是方便查看图片裁剪的结果。

##本文档主要参考文章：
- 如何生成`cascade.xml`方面  
`https://github.com/mrnugget/opencv-haar-classifier-training`
- 如何使用`cascade.xml`方面  
`https://github.com/zeusees/HyperLPR.git`
- 训练haar级连  
`https://memememememememe.me/post/training-haar-cascades/`  
- 将数据集训练为XML文件以进行Cascade分类器OpenCV  
`https://medium.com/@toshyraf/train-dataset-to-xml-file-for-cascade-classifier-opencv-43a692b74bfe`  

##主要步骤：
> 以下步骤都是在文件夹`opencv-haar-classifier-training`中进行  
> 1.Put your positive images in the ./positive_images folder and create a list of them:  
>> `find ./positive_images -iname "*.jpg" > positives.txt`

> 2.Put the negative images in the ./negative_images folder and create a list of them:
>> `find ./negative_images -iname "*.jpg" > negatives.txt`

> 3.Create positive samples with the bin/createsamples.pl script and save them to the ./samples folder: 
>> `perl bin/createsamples.pl positives.txt negatives.txt samples 1500\`
`"opencv_createsamples -bgcolor 0 -bgthresh 0 -maxxangle 1.1\`
`-maxyangle 1.1 maxzangle 0.5 -maxidev 40 -w 80 -h 40"`

> 4.Use tools/mergevec.py to merge the samples in ./samples into one file:
>> `python ./tools/mergevec.py -v samples/ -o samples.vec`  
>> Note: If you get the error struct.error: unpack requires a string argument of length 12 then go into your samples directory and delete all files of length 0.

> 5.0.Start training the classifier with opencv_traincascade, which comes with OpenCV, and save the results to ./classifier:
>> `opencv_traincascade -data classifier -vec samples.vec -bg negatives.txt\`
`-numStages 20 -minHitRate 0.999 -maxFalseAlarmRate 0.5 -numPos 1000\`
`-numNeg 600 -w 80 -h 40 -mode ALL -precalcValBufSize 1024\`
`-precalcIdxBufSize 1024`

> 5.1.If you want to train it faster, configure feature type option with LBP:
>> `opencv_traincascade -data classifier -vec samples.vec -bg negatives.txt\`
`-numStages 20 -minHitRate 0.999 -maxFalseAlarmRate 0.5 -numPos 1000\`
`-numNeg 600 -w 80 -h 40 -mode ALL -precalcValBufSize 1024\`
`-precalcIdxBufSize 1024 -featureType LBP`

## 进行测试
- 该测试的程序是参考HyperLPR的Linux程序。
- 在build目录下执行`cmake ..`
- 在build目录下执行`make`，会在上层目录中生成TEST_Detection可执行文件。
- 执行可执行文件`./TEST_Detection res/test07.jpg 80 80`如果成功则会看到出现了`cascaxx.jpg`文件。
