import 'dart:async';
import 'package:flutter/material.dart';
import 'package:intl/intl.dart';
import 'package:multibletest_hmi/unit_customBLE.dart';

import 'main.dart';

// 상수 정의
const List<String> bleNames = [
  'BLEMultiTest_01',
  'BLEMultiTest_02',
  'BLEMultiTest_03',
  'BLEMultiTest_04',
  'BLEMultiTest_05',
  'BLEMultiTest_06',
];

const List<String> uuidList = [
  '92c968b1-d59b-41f3-bf8e-a2bd383dc73e',
  '10d99883-46e1-486c-8dc6-f1761cd3a969',
  '898a1e7c-10be-4c26-a0b0-92a98300e7a6',
  '1e89e025-fa87-44b8-8ea9-e94928e3d110',
  'fd89530d-92d4-4211-b1e9-d1277b603dd3',
  '0e71b325-a05f-4336-9156-d3ee1bb3f6c3',
];

const List<String> rxCharList = [
  'daa09ad3-d45a-44b5-9624-feac2213f93f',
  '4f95c09a-f6ad-45e3-8afb-7e9e4afd10e0',
  '1dbd7549-4d70-49c6-957f-dd72cec59da8',
  'bc7ef623-548a-43ac-80ac-f7d9446c9376',
  '75cb2362-a085-4d54-8be5-e92fb31ab001',
  'fcd0cc37-8a44-43b1-bfed-fd94c597a6d4',
];

const List<String> txCharList = [
  'b419d0e6-a538-4416-8451-e47522a55ee6',
  '57e4dfaf-d4f4-4789-9f0f-9d14672942e9',
  '0c27cd56-d68a-4135-8b61-214b39a747e3',
  '326c6d45-d167-4c53-9b69-ec3d813f5cf1',
  'd0bf1f15-8bef-4725-b884-da28968fb7ff',
  '4e7661ba-afee-4384-be1e-3a6a631135d4',
];

class PageHomeState extends State<PageHome> {
  List<String> sendData = List.generate(6, (_) => '');
  List<int> setData = List.generate(6, (_) => 0);
  List<int> setchar = List.generate(6, (_) => 0);
  List<DateTime> soyoTime = List.generate(6, (_) => DateTime.now());
  DateTime? startTime;
  DateTime? endTime;
  String name = '';

  late List<CustomBLE> myBLE = List.generate(6, (index) {
    return CustomBLE(
      name: bleNames[index],
      bleSetting: BLESettings(
        uuid: uuidList[index],
        rxChar: rxCharList[index],
        txChar: txCharList[index],
      ),
      fetchData: (data) {
        DateTime now = DateTime.now();
        int count = int.tryParse(data.split(':')[1]) ?? 0;

        setchar[index] = count - setData[index];

        setData[index] = count;
        sendData[index] = data;
        soyoTime[index] = now;
      },
    );
  });

  late Timer timer;
  int set = 6;

  @override
  void initState() {
    super.initState();
    timer = Timer.periodic(Duration(milliseconds: 10), (timer) {
      bool allConnected = true;
      for (int i = 0; i < 6; i++) {
        if (!myBLE[i].isConnected()) {
          allConnected = false;
          sendData[i] = '';
        }
      }
      if (allConnected && endTime == null) {
        endTime = DateTime.now();
      }
      setState(() {});
    });
  }

  @override
  void dispose() {
    timer.cancel();
    super.dispose();
  }

  Future<void> start() async {
    for (int i = 0; i < myBLE.length; i++) {
      set = i;
      if (!myBLE[i].isConnected()) {
        name = '${i + 1}번 연결중';
        await myBLE[i].connect();
        if (myBLE[i].isConnected()) {
          name = '${i + 1}번 연결성공';
        } else {
          name = '${i + 1}번 연결실패';
        }
        await Future.delayed(Duration(seconds: 1));
      }
      name = '';
    }
    set = 6;
    await Future.delayed(Duration(seconds: 1));
    start();
  }

  Widget buildListTile(int index) {
    return ListTile(
        leading: Icon([Icons.looks_one, Icons.looks_two, Icons.looks_3, Icons.looks_4, Icons.looks_5, Icons.looks_6][index]),
        title: Text(sendData[index]),
        subtitle: Text('${myBLE[index].isConnected()
            ? '연결됨'
            : myBLE[index].isTryToConnect()
            ? '연결중'
            : '연결 안됨'} - (${setchar[index]})'),
        trailing: Text(myBLE[index].isConnected() && DateTime.now().difference(soyoTime[index]).inMilliseconds >= 250 ? '연결끊김\n${NumberFormat('#,##0.000').format(DateTime.now().difference(soyoTime[index]).inMilliseconds / 1000)}초 전' : '', textAlign: TextAlign.center));
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
        appBar: AppBar(
          title: Text(
            '다중 BLE 테스트 어플',
            style: TextStyle(fontWeight: FontWeight.bold),
          ),
          leading: Icon([Icons.looks_one, Icons.looks_two, Icons.looks_3, Icons.looks_4, Icons.looks_5, Icons.looks_6, Icons.calculate_rounded][set]),
        ),
        body: Column(children: [
          Text(name,
              style: TextStyle(
                  fontWeight: FontWeight.bold,
                  color: name.endsWith('공')
                      ? Colors.greenAccent
                      : name.endsWith('패')
                          ? Colors.redAccent
                          : Colors.black)),
          ...List.generate(6, buildListTile),
          Spacer(),
          if (endTime != null) Card(child: Padding(padding: EdgeInsets.all(50), child: Row(children: [Center(child: Text('연결 소요시간 : ${NumberFormat('#,###').format(endTime!.difference(startTime!).inMilliseconds)}㎳'))])))
        ]),
        floatingActionButton: (startTime == null)
            ? FloatingActionButton.extended(
                onPressed: () {
                  startTime = DateTime.now();
                  start();
                },
                label: Text('시작하기'),
                icon: Icon(Icons.play_circle_fill))
            : null);
  }
}
