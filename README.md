# RM_MEC_and_5G_FYP2025_asia

File IOT in scratch and this for test only not in our project

1- Create file iot-mec.cc in scratch directory
2- Create other file in the same directory in ns3 files
3- Add to the CMakeLists.txt in network directpry  
    model/iot-device.cc
    model/iot-device.h
4- Add to the CMakeLists.txt in applications directpry  
    model/mec-server.cc
    model/mec-server.h
5- run the project in path "ns-allinone-3.43/ns-3.43"
    ./ns3 run iot-mec
6- run the Animation in path "ns-allinone-3.43/netanim-3.109"
    ./NetAnim

--------------------------------------------------------------------
اخترت src/network/model/ كخيار أولي لأن أجهزة IoT هي عقد شبكية تتصل بالبنية التحتية، وهذا يتماشى مع الهيكل التنظيمي لـ ns-3، حيث يتم وضع النماذج المرتبطة بالشبكة في هذا المجلد
1️⃣ أجهزة IoT تمثل عقدًا شبكية تتصل بالسيرفر باستخدام بروتوكولات شبكة.
2️⃣ تحتوي src/network/model/ بالفعل على نماذج مرتبطة بالبنية الشبكية مثل point-to-point و**wifi**، مما يجعلها مكانًا طبيعيًا لمكونات IoT.
3️⃣ لا تحتوي الأجهزة نفسها على تطبيقات معقدة، بل هي أجهزة تقوم بنقل البيانات، مما يجعلها أقرب إلى مكونات الشبكة.
--------------------------------------------------------------------
