# process prototype sensor data from ESP8266 for multiple sensors
# Jordan Stark July 2019

# set input and output dirs
outpath = "C:/Users/Jordan/Desktop/Aug2019 TripSensorsDeployment/Aug2019Data/Routput"
datapath = "c:/Users/Jordan/Desktop/Aug2019 TripSensorsDeployment/Aug2019Data/clean/"

# files should be .log text files in the folder listed as "datapath"
# before running the script, remove the file headers and row numbers
# I'm using a notepad++ macro that also removes blank rows and error messages

# packages
library(lubridate)
library(tidyr)
library(ggplot2)
library(dplyr)
library(purrr)

sensorlist <- c("A19","A16","A31","A10","A24") #sensors to include -- should be the first part of the file name
filenames <- paste(sensorlist,"_8Aug19.log",sep="") # names of files in datapath folder


# import data - this is a pre-cleaned CSV from PuTTY log
allraw <- vector("list",length(sensorlist))

for(i in 1:length(sensorlist)){
  allraw[[i]] <- read.csv(paste(datapath,filenames[i],sep=""),header=F,stringsAsFactors=T)
  allraw[[i]][,14] <- sensorlist[i]
  names(allraw[[i]]) <- c("y","mo","d","h","m","vmc2_r","vmc3_r","lux","soiltemp","pres","temp","hum","power","ID")
} #check to make sure that the data are actually in this order!



longdatalist <- vector("list",length(sensorlist))

for(i in 1:length(sensorlist)){
  calcs <- allraw[[i]]
  
  # calculate real date and time from ymdhm columns
  
  calcs$timestamp <- ymd_hm(paste(paste(calcs$y,calcs$mo,calcs$d,sep="-"),paste(calcs$h,calcs$m,sep = ":")))
  calcs$hm <- hour(calcs$timestamp)+minute(calcs$timestamp)/60
  calcs$date <- factor(paste(calcs$y,calcs$mo,calcs$d))
  
  # calculate par, vmc, voltage (from Mickley et al 2019)
  calcs$vmc_10cm <- 0.52 - 2.46*10^-5 * calcs$vmc2 + 2.54*10^-10 * calcs$vmc2^2
  calcs$vmc_surface <- 0.52 - 2.46*10^-5 * calcs$vmc3 + 2.54*10^-10 * calcs$vmc3^2
  calcs$par <- 22 + (0.019 * calcs$lux) - (1.90*10^-8) * calcs$lux^2
  calcs$volts <- calcs$power/1024 * 6.5
  
  # data cleaning - removing suspicious values. Currently very arbitrary. Edit!
  calcs <- calcs[-which(calcs$m!=0),] #remove anything not read on the hour
  #calcs[1,] <- NA
  #calcs[length(row.names(calcs)),] <- NA
  #calcs$vmc0[calcs$vmc0>0.1] <- NA
  #calcs$vmc1[calcs$vmc1>0.1] <- NA
  #calcs$par1[calcs$lux1<0]   <- NA
  #calcs$par2[calcs$lux2<0]   <- NA
  #calcs$hum[calcs$hum<10]    <- NA
  #calcs$temp[calcs$temp<10]  <- NA
  #calcs$volts[calcs$volts<5] <- NA
  
  
  # create data frame with just final values
  cleandata <- data.frame(timestamp=calcs$timestamp,
                          pres=calcs$pres,
                          temp=calcs$temp,
                          hum=calcs$hum,
                          soiltemp=calcs$soiltemp,
                          volts=calcs$volts,
                          vmc_10cm=calcs$vmc_10cm,
                          vmc_surface=calcs$vmc_surface,
                          par=calcs$par
                          )
  
  
  longdata <- gather(cleandata,datatype,meas,-timestamp)
  longdata$sensorID <- calcs$ID[1]
  
  longdatalist[[i]] <- longdata

}



# combine data by sensor

bysensor <- do.call(rbind,longdatalist)
baddates2 <- bysensor$timestamp<ymd("2019-01-01")
bysensor <- bysensor[!baddates2==TRUE,]
bysensor <- bysensor[!is.na(bysensor$meas),]
bysensor <- bysensor[!is.na(bysensor$sensorID),]

ggplot(bysensor,aes(x=timestamp,y=meas,color=sensorID))+
  geom_point()+
  facet_grid(rows=vars(datatype),scales="free",switch="y") +
  theme_bw() +
  theme(axis.title.y=element_blank(),strip.placement="outside",strip.background=element_blank())

#ggsave(paste(outpath,"/sensorsinlab_11Aug19.pdf",sep=""))

ggplot(bysensor[bysensor$sensorID=="A19",],aes(x=timestamp,y=meas))+
  geom_point()+
  facet_grid(rows=vars(datatype),scales="free",switch="y") +
  theme_bw() +
  theme(axis.title.y=element_blank(),strip.placement="outside",strip.background=element_blank())

#ggsave(paste(outpath,"/sensorA05_11Aug19.pdf,sep=""))





