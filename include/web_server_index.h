const char MAIN_page[] PROGMEM = R"=====(
<!doctype html>
<html>

<head>
	<title>SmartCoffee</title>
	<!--For offline ESP graphs see this tutorial https://circuits4you.com/2018/03/10/esp8266-jquery-and-ajax-web-server/ -->
	<script src = "https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.7.3/Chart.min.js"></script>
	<style>
	canvas{
		-moz-user-select: none;
		-webkit-user-select: none;
		-ms-user-select: none;
	}

	/* Data Table Styling */
	#dataTable {
	  font-family: "Trebuchet MS", Arial, Helvetica, sans-serif;
	  border-collapse: collapse;
	  width: 100%;
	}

	#dataTable td, #dataTable th {
	  border: 1px solid #ddd;
	  padding: 8px;
	}

	#dataTable tr:nth-child(even){background-color: #f2f2f2;}

	#dataTable tr:hover {background-color: #ddd;}

	#dataTable th {
	  padding-top: 12px;
	  padding-bottom: 12px;
	  text-align: left;
	  background-color: #4CAF50;
	  color: white;
	}
	</style>
</head>

<body>
    <div style="text-align:center;"><b>SmartCoffee</b></div>
		<br>
		<div align="center"><br>
			<table>
				<tr align="left">
					<td>Power</td>
					<td><b id='OnOffState'>unknown</b></td>
					<td>
						<form target="hidden" method="GET" action="/setValues">
							<INPUT TYPE=SUBMIT VALUE="Switch" name=OnOff>
						</form>
					</td>
				</tr>
				<tr align="left">
					<td>Brewing</td>
					<td></td>
					<td>
						<form target="hidden" method="GET" action="/setValues">
							<INPUT TYPE=SUBMIT VALUE="start" name=startBrewing>
						</form>
					</td>
				</tr>

				<tr>
					<td>Setpoint</td>
					<td></td>
					<td>
						<form target="hidden" method="GET" action="/setValues">
							<INPUT TYPE=text name=SetPoint VALUE="0">
						</form>
					</td>
				</tr>
				<tr>
					<td>StateMachine</td>
					<td><b id="StateMachine">unknown</b></td>
					<td></td>
				</tr>
				<tr>
					<td>Brewing Timer</td>
					<td><b id="BrewingTimer">unknown</b></td>
					<td></td>
				</tr>
			</table>
		</div>


    <div class="chart-container" position: relative; height:350px; width:100%">
        <canvas id="Chart" width="400" height="400"></canvas>
    </div>

		<div>
			<table id="dataTable">
			  <tr><th>Time</th><th>Temp Value</th><th>Temp Average Value</th></tr>
			</table>
		</div>
<br>
<br>

<iframe name="hidden" href="about:blank" style="display:none"></iframe>

// ========================================================================================================================================================
// ========================================================================================================================================================
<script>
//Graphs visit: https://www.chartjs.org
var TempSensValues = [];
var TempAvgSensValues = [];
var SetPointValues = [];
var PIDValues = [];
var timeStamp = [];
var counter = 0;
var OnOffState = "unknown";
var StateMachine = "unknown";
var BrewingTimer = "unknown"

function showGraph() {
    var ctx = document.getElementById("Chart").getContext('2d');
    var Chart1 = new Chart(ctx, {
        type: 'line',
        data: {
            labels: timeStamp,  //Bottom Labeling
            datasets: [{
                label: "Sensor",
                fill: false,  //Try with true
                backgroundColor: 'rgba( 243, 18, 156 , 1)', //Dot marker color
                borderColor: 'rgba( 243, 18, 156 , 1)', //Graph Line Color
                data: TempSensValues,
            },{
                label: "SensorAverge",
                fill: false,  //Try with true
                backgroundColor: 'rgba( 243, 156, 18 , 1)', //Dot marker color
                borderColor: 'rgba( 243, 156, 18 , 1)', //Graph Line Color
                data: TempAvgSensValues,
            },{
                label: "SetPoint",
                fill: false,  //Try with true
                backgroundColor: 'rgba( 18, 243, 156 , 1)', //Dot marker color
                borderColor: 'rgba( 18, 243, 156 , 1)', //Graph Line Color
                data: SetPointValues,
            },{
                label: "PID",
                fill: false,  //Try with true
                backgroundColor: 'rgba( 255, 0, 0 , 1)', //Dot marker color
                borderColor: 'rgba( 255, 0, 0 , 1)', //Graph Line Color
                data: PIDValues,
            }],
        },
        options: {
            title: {
                    display: true,
                    text: "Temperature"
                },
						animation: false,
            maintainAspectRatio: false,
            elements: {
            line: {
                    tension: 0.5 //Smoothening (Curved) of data lines
                }
            },
            scales: {
                    xAxes: [{
                        ticks: {
                            beginAtZero:true,
														suggestedMin: 0,
														suggestedMax: 120
                        }
                    }],
										yAxes: [{
	                      ticks: {
	                          beginAtZero:true,
														suggestedMin: 0,
				                    suggestedMax: 130,
														stepSize: 10
	                      }
	                  }]
            }
        }
    });
}

function updateStates() {
	document.getElementById("OnOffState").innerHTML = OnOffState;
	document.getElementById("StateMachine").innerHTML = StateMachine;
	document.getElementById("BrewingTimer").innerHTML = BrewingTimer;
}
//On Page load show graphs
window.onload = function() {
  console.log(new Date().toLocaleTimeString());
};

//Ajax script to get ADC voltage at every 5 Seconds
//Read This tutorial https://circuits4you.com/2018/02/04/esp8266-ajax-update-part-of-web-page-without-refreshing/

setInterval(function() {
  // Call a function repetatively with 5 Second interval
  getData();
}, 1000); //5000mSeconds update rate

function getData() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
     //Push the data in array
		  var time = counter;//new Date().toLocaleTimeString();
			counter++;
		  var txt = this.responseText;
		  var obj = JSON.parse(txt); //Ref: https://www.w3schools.com/js/js_json_parse.asp

			if(obj.resetGraph == "true") {
				TempSensValues = [];
				TempAvgSensValues = [];
				SetPointValues = [];
				PIDValues = [];
				timeStamp = [];
				counter = 0;
				time = 0;
			}
	    TempSensValues.push(obj.TempSens);
	    TempAvgSensValues.push(obj.TempAvgSens);
			SetPointValues.push(obj.SetPoint);
			PIDValues.push(obj.PID);
	    timeStamp.push(time);
    	showGraph();  //Update Graphs

			OnOffState = obj.OnOffState;
			StateMachine = obj.StateMachine;
			BrewingTimer = obj.BrewingTimer;
			updateStates();

		//Update Data Table
	    var table = document.getElementById("dataTable");
	    var row = table.insertRow(1); //Add after headings
	    var cell1 = row.insertCell(0);
	    var cell2 = row.insertCell(1);
	    var cell3 = row.insertCell(2);
	    cell1.innerHTML = time;
	    cell2.innerHTML = obj.TempSens;
	    cell3.innerHTML = obj.TempAvgSens;
    }
  };
  xhttp.open("GET", "getValues", true); //Handle readADC server on ESP8266
  xhttp.send();
}

</script>
</body>

</html>

)=====";
