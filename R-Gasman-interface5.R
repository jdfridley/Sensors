#Gasman Control Code, revised Fridley Feb 2019

#-------------Notes
#**Numato relay control requires global env path installation of python, which is invoked by R, along with pyserial library.
#1. Interacts directly with Numato 32-relay board via serial connection. Replaces Arduino code.
#2. Serial connection does not (yet) work through R serial package. Instead serial connection invoked in python with pyserial package, invoked in R with reticulate package.
#3. Includes serial interactivity with Gasman Env Sensor readings (PAR, temp, RH) and Li-cor 6262
#4. Licor 6262 serial output (function 13) must be set to 1: 23 2:32 3:42
#5. Original code for R-Arduino interface, March 2018, modified Oct 2018.

#-----------Libraries
	
	library(svDialogs) #for convenient dialog input boxes
	library(serial) #serial package allows writing to serial port
	library(stringr) #for reading in complicated strings
	library(reticulate) #runs python in background, for relay serial connection

#-----------Serial Connections

	#numato relay control via USBserial
	py_available(initialize=T) #start python in background
	py_run_string("import serial") #start python serial package
	py_run_string("ser = serial.Serial('COM6', 19200, timeout=1)") #open relay port for PC
	#py_run_string("ser = serial.Serial('/dev/cu.usbmodem1411', 19200, timeout=1)")
		#open relay port (on Mac)
	
	#Gasman env sensor serial connection via USB
	envcon <- serialConnection(name = "testcon",port = "COM7",mode = "115200,n,8,1",newline = 1,translation="lf")
	#Licor 6262 serial connection; PC is a COM port 9600 baud
	con <- serialConnection(name = "testcon",port = "COM5",mode = "9600,n,8,1",newline = 1,translation="lf")
	#con <- serialConnection(name = "testcon",port = "cu.usbmodem1421",mode = "9600,n,8,1",newline = 1,translation="lf") #mac version

#-----------Relay control functions
	#specific to Numato 32 relay board
	
	#32 relay IDs in the order of 0-9 then A:V:
	rvec = c(0:9,LETTERS[1:22])
	
	#separate on and off functions by relay number
	relon = function(rno) {
		stron = paste("ser.write(b'relay on ",rvec[rno],"\\r')",sep="")
		py_run_string(stron)
	}
	reloff = function(rno) {
		stron = paste("ser.write(b'relay off ",rvec[rno],"\\r')",sep="")
		py_run_string(stron)
	}
	
	#mapping of chamber lids and valves to associated relay switches
	#relay numbers of lid down circuits, in order from chamber 1 to 8
	lid = c(10,11,20,17,7,5,32,30) #eg relon(lid[1]) closes relay 10 (code 9) conn to lid 1
	#relay numnbers of aboveground valves, in order from chamber 1 to 8
	aval = c(14,15,13,16,1,4,3,2) 
	#relay numnbers of belowground valves, in order from chamber 1 to 8
	bval = c(24,23,22,21,27,25,28,26)

#-----------Data files
	outfile = paste("test",Sys.Date(),".txt",sep="")  #default output filename
	outfile2 = dlgSave(default=outfile,title="Filename for output data:")$res
	#for network saving:
	net.dir = "G:\\Private\\gasman\\"  #location on SU active directory (ensure permissions)
	chosen.file =  substr(outfile2,gregexpr("/",outfile2)[[1]][3]+1,100) #filename output from dlgSave
	outfile3 = paste(net.dir,chosen.file,sep="") #file name and path for network saving
	#output dataframe
		plant.lab = NULL
		time.lab = NULL
		slope.lab = NULL
		R2.lab = NULL
		abcham.lab = NULL
		df = data.frame(plant.lab,time.lab,slope.lab,R2.lab,abcham.lab)
		cnames = c("Plant","DateTime","Rate","R2","Chamber")
		write.table(df,file=outfile)
	j = NULL	#ultimate time tracker, from start to finish of run
	CO2j = NULL #single CO2 vector for full run
	PARj = NULL #single PAR vector for full run
	H2Oj = NULL #single H2O vector for full run
	tCj = NULL  #single licor temp (C) vector for full run

#-----------Control prompts

	wait = 5 	#seconds to wait between A and B measurements
	nObs <- 40 	#seconds of monitoring each chamber

	#chamber selection
	chamb.list = c("Above and belowground","Above only","Below only")
	#chamb = "Above and belowground"
	chamb <- dlgList(chamb.list, multiple = FALSE ,title="Select chamber method:")$res

	#keep looping foreover?
	loop.list = c("Single Run","Loop forever")
	loopfor = dlgList(loop.list,multiple=FALSE,title="Loop over and over?")$res

	#chamber order input
	warn=0
	while(warn==0) {
	Clist = dlgInput(message="Enter chamber order separated by commas:")$res
	if(Clist == "") stop("No chambers entered") #stops script if nothing entered
		clist = as.numeric(unlist(strsplit(Clist, split=","))) #converts string to numbers
		if(sum(is.na(clist))==0) break
	dlgMessage("Nonstandard chamber specification, try again", type = c("ok"), gui = .GUI)
	}

#open(con) #open serial connection to licor
#open(envcon) #open gasman env sensor communication

#------------------------------------------------------
#Forever looping
stop.loop = 0
while(stop.loop==0) { #keep looping unless stop.loop changes value

#------------------------------------------------------
#Plant loop p
for(p in 1:length(clist)) {
	plant = clist[p]

#LOG CO2 DATA---------------------------------------------
ab.lab = c("Aboveground","Belowground")
for(ab in 1:2) { #loop over above then belowground measurements
	if(ab==1&chamb=="Below only") next #skip aboveground if so chosen
	if(ab==2&chamb=="Above only") next #skip belowground if so chosen
	
	CO2 <- rep(NA, nObs) #CO2 ppm input vector from licor
	H2O <- rep(NA, nObs) #H2O ppt input vector from licor
	tC <- rep(NA, nObs)  #temp in C input vector from licor
	sec <- rep(NA, nObs) #time vector
	par <- rep(NA, nObs) #light vector
	temp <- rep(NA, nObs) #outside chamber temp vector
	rh <- rep(NA, nObs) #outside chamber RH vector

	#switch on relays
	if(ab==1) {relon(lid[plant]); relon(aval[plant])}
	if(ab==2) {relon(bval[plant])}

	#NEW: open and close serial data ports within loop
	open(con) #open serial connection to licor
	open(envcon) #open gasman env sensor communication
	Sys.sleep(1)

	i = 1
	time = Sys.time() #time that logging starts
	while(i < nObs) {
		input = try(as.numeric(str_extract_all(read.serialConnection(con),"\\(?[0-9,.]+\\)?")[[1]][1:3])) #CO2,H20,temp from licor
    		if(class(input[1])=="try-error") {Sys.sleep(.5); next} #catch error and get new value
		Sys.sleep(.5) #inserted because otherwise script runs too fast for new con value
    		if(is.na(input[1])) {Sys.sleep(.5); next} #weird inputs skipped
    		CO2[i] <- input[1]
		H2O[i] <- input[2]
		tC[i] <- input[3]
		sec[i] <- Sys.time()
		if(length(j)==0) time.zero = Sys.time() #time of first measurement
		if(i>1) sec[i] = sec[i] - sec[1] 	#time is relative, my friend
		if(length(j)>0) j = c(j,as.numeric(difftime(Sys.time(),time.zero,units="sec"))) else j=0
		CO2j = c(CO2j,CO2[i])			#ultimate CO2 vector
		H2Oj = c(H2Oj,H2O[i])			#ultimate H2O vector
		tCj = c(tCj,tC[i])			#ultimate temp vector (licor)

		#env data
		stopx = 0
		while(stopx!=1) {
			inputE = read.serialConnection(envcon)
			if(is.null(inputE)) next
			if(inputE=="") next
			stopx = 1
		}
		invec = as.numeric(strsplit(substr(inputE,1,regexpr("\r",inputE)-1),split=",")[[1]])
		par[i] = invec[1]
		temp[i] = invec[2]
		rh[i] = invec[3]
		PARj = c(PARj,par[i])			#ultimate PAR vector

		if(length(j)>1) {par(mar=c(3,4,5,5)); plot(j[-1],CO2j[-1],ylim=c(550,720),ylab="CO2 ppm",xlab="",pch=19,cex.lab=1.3,cex.axis=1.3)
		mtext(paste("Plant ",plant,": ",ab.lab[ab]," flux"),side=3,cex=1.5,line=2)
		par(new = T)
		plot(j[-1],PARj[-1],axes=F,xlab=NA,ylab=NA,col="darkgreen",pch=19,cex=1)
		axis(side=4,line=.2,cex.axis=.7,col="darkgreen",col.ticks="darkgreen"); mtext(side = 4, line = .5,'PAR',col="darkgreen",cex=1)
		par(new = T)
		plot(j[-1],H2Oj[-1],axes=F,xlab=NA,ylab=NA,col="blue",pch=20,cex=1.2)
		axis(side=4,line=2.5,cex.axis=.7,col="blue",col.ticks="blue"); mtext(side = 4, line = 2.6,'H2O ppt',col="blue",cex=1)
		}
    		#if(i>5) { #calculate slope once enough points recorded
    		#	x = c(1:i)
    		#	y = CO2[1:i]
    		#	lm1 = lm(y~x)
    		#	slope = round(lm1$coef[2],3)
    		#	R2 = round(summary(lm1)[[8]],3)
    		#	mtext(paste("Slope=",slope),side=3,at=5,line=.3,cex=1.3)
    		#	mtext(paste("R2=",R2),side=3,at=25,line=.3,cex=1.3)
    		#}

		i = i+1
    		Sys.sleep(.5) #minimizes conflicts of read intervals
	}
	#append data to file once all readings for given chamber are complete (local and network)
	write.table(data.frame(sec,CO2,plant,c("A","B")[ab],par,temp,rh,H2O,tC),outfile2,append=T,col.names=F)
	try(write.table(data.frame(sec,CO2,plant,c("A","B")[ab],par,temp,rh,H2O,tC),outfile3,append=T,col.names=F))

	#NEW: close serial data ports within loop
	close(con) #close serial connection
	close(envcon) #close serial connection

	#turn off relays
	if(ab==1) {reloff(lid[plant]); reloff(aval[plant])}
	if(ab==2) {reloff(bval[plant])}	

	Sys.sleep(wait) #seconds to wait between valve shifts
} #close ab loop

} #close plant loop
if(loopfor!="Loop forever") stop.loop = 1 	#if single run, run stops here
} #close forever loop

#ensure relays off
if(ab==1) {reloff(lid[plant]); reloff(aval[plant])}
if(ab==2) {reloff(bval[plant])}	

#close relay port
py_run_string("ser.close()") #close relay port

#close(con) #close serial connection
#close(envcon) #close serial connection




