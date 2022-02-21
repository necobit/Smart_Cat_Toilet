unction doPost(e) {

  var sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName('シート1');
  
  // 送信されてくるJSONデータから、各要素を取り出す
  var params = JSON.parse(e.postData.getDataAsString());
  var value = params.value;
  var wdiff = params.wdiff;

  // データをシートに追加
  sheet.insertRows(2,1);  // 2行1列目に列を挿入する

  // var range = sheet.getRange("A2:C2");
  // var array = [new Date(),value,wdiff];
  // range.setValue(array);


 sheet.getRange(2, 1).setValue(new Date());     // 受信日時を記録
 sheet.getRange(2, 2).setValue(value);     // 重量を記録
 sheet.getRange(2, 3).setValue(wdiff);     // 重量を記録
 
}