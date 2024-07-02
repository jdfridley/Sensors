#Calculation of Rd.area in a closed chamber using the LI-850
#Includes batch LI-850 text file processing
#Fridley & Martineau 2024

#Technical notes
	#the LI-850 does account for water to determine [CO2] of the air (ie, the IR absorption issue)
	#the LI-850 does NOT automatically account for the dilution effect of water increase on CO2 flux
	#see this technical note:
	#https://licor.app.boxenterprise.net/s/igs56gijkc4ftks30pci
	#the below calculation includes the dilution effect and the dead band at the start of the measurement 
	#however it assumes that dry CO2 increase is linear over the period measured;
	#ie, that increasing CO2 in the air column does not significantly reduce the flux rate
	#this assumption is addressed empirically below to confirm

#Batch processing
	#Change this path to your folder that contains all the LI-850 text files (and ONLY those files)
	#Filenames should match the 'tag' column in the spreadsheet below
		#example: sample with tag '107' should have a LI-850 file saved as "107.txt"
	folder = c("C:\\Users\\Jenna Martineau\\OneDrive - Clemson University\\Documents\\Clemson University\\Rd SCBG summer 2023\\")	
	filenames = list.files(folder)
	#N = length(filenames) #total number of files

#Csv file with associated metadata for each sample
	#below, we assume sample labels are in a column called 'tag'
	#we also assume this file has columns for:
		#sample leaf area in cm2 ('area') 
		#LI-850 measuremental interval ('interval'; typically 1 s)
		#chamber volume ('vol') in mL (ie, this can vary by sample if different chambers are used)	

	leafstats = read.csv("C:\\Users\\Jenna Martineau\\OneDrive - Clemson University\\Desktop\\Rd2023re.csv")
	N = dim(leafstats)[1] #number of samples

#Prepare output column (new column in leafstats)

	leafstats$Rd.out = NA 
	leafstats$Rd.se = NA #standard error of estimate

#If needed, specify labels of samples in 'leafstats' that do not have associated LI-850 files (these will be ignored below)

	ignore.tags = c("") #if none to ignore
	#ignore.tags = c(109,224,227,263,280,340,367,388,416,405)	

#Loop over each file, calculate Rd.out

for(i in 1:N) {

  print(i) #keep track of progress

  ftag = leafstats$tag[i] #select sample from leafstats
	if(is.element(ftag,ignore.tags)) next  #skip sample if missing LI-850 file
  
  #constants
  vol = leafstats$vol[leafstats$tag==ftag]  #chamber volume in mL
  R = 8.314 #gas constant, ( JK-1mol-1)
  T = 21 #chamber temperature, deg C
  flow = 750 #flow rate, mL per min  
  area = leafstats$area[leafstats$tag==ftag] #leaf area in cm2
  interval = leafstats$interval[leafstats$tag==ftag] #measurement interval
    
  #data file
  dat = read.table(paste0(folder,ftag,".txt"),skip=1,header=T)
  names(dat) = c("date","time","co2ppm","h20ppt","h20","celltemp","cellpress","co2ab","h20ab","volt","flow")
  dat$sec = 1:dim(dat)[1] * interval #creates proper time scale
  
  #throw away 'dead air' time: amount of time it takes to equilibrate air stream
  deadtime = (vol/flow) * 60
  deadtime = 80	#manual if needed
  dat2 = dat[-c(1:deadtime),]
  
  #calculate Rdarea using licor equation, with slope calculated after dead time
  	#using licor equation: https://www.licor.com/env/support/LI-6800/topics/soil-chamber-theory.html#Theoryandequationsummary

  dat2$co2dry = dat2$co2ppm / ( 1 - (dat2$h20ppt/1000)) 
  dryslope = coef(lm(co2dry~sec,dat2))[2]  #non-diluted rate of CO2 increase (0% humidity)
  F = ((10 * vol) * dat$cellpress[1] * (1 - (dat$h20ppt[1]/1000)) * dryslope) / (R * area * (T+273.15))
  
  #plot everything just to make sure, red line is associated wet slope after dead time
  plot(dat$sec,dat$co2ppm,main=paste0(ftag,", Rd=",round(F,3)))
  abline(lsfit(dat2$sec,dat2$co2ppm),col="red")    
  readline() #pauses, waits for enter - comment out if needed
  
  leafstats$Rd.out[i] = F #saves expected Rd value
  
  #calculate standard error of Rd measurement
  	#error comes from dryslope coefficient, but need to put in terms of Rd.area
  	#randomly sample dryslope from a normal distribution using mean and SE of dryslope, then convert values to Rd.area and take SD
	
	lm1= (lm(co2dry~sec,dat2))
	RdmeanMCdry = rnorm(1000,mean=summary(lm1)[[4]][2,1],sd=summary(lm1)[[4]][2,2])
	RdmeanMCwet = ((10 * vol) * dat$cellpress[1] * (1 - (dat$h20ppt[1]/1000)) * RdmeanMCdry) / (R * area * (T+273.15))
	leafstats$Rd.se[i] = sd(RdmeanMCwet)
	  
}
