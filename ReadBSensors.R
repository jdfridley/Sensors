# Script to read in data from 'B' version EMUs
# including calibration for soil moisture

# Jordan Stark, Spring 2021


#### Setup ####
# packages
  library(lubridate)
  library(stringr)


# paths
  data_path <-  "C:/Users/Jordan/Desktop/Smokies Data/FieldData/B_sensors/"
    # this should contain all raw sensor data (and nothing else)
  Metadata_path <- "C:/Users/Jordan/Desktop/Smokies Data/"
    # this should contain calib_coefs.csv
  Out_path <- "C:/Users/Jordan/Desktop/Smokies Data/CleanSoilData/"
    # location to save outputs


#### function to read raw sensor data ####
    Bcleaner <- function(dat) {
      # remove text lines from data
        errlines <- str_detect(dat,paste("err","loop","read",sep="|"))
        
        cleandat <- dat[errlines==F]
        cleandat <- str_split(cleandat,":",simplify=T)[,2] # remove line numbers
        cleandat <- cleandat[2:length(cleandat)] # remove header
      
      # convert to data frame
        alldf <- data.frame(str_split(cleandat,",",simplify=T),stringsAsFactors=F)
        names(alldf) <- c("y","mo","d","h","m","vmc_Deep_raw","vmc_Surf_raw","soiltemp","power","SensorID")
      
      # set numeric format for all columns except ID
        alldf[,1:9] <- sapply(alldf[,1:9],as.numeric)
      
      
      # remove NA values and format timestamp
        alldf$timestamp <- ymd_hm(paste(alldf$y,"/",alldf$mo,"/",alldf$d," ",alldf$h,":",alldf$m,sep=""),quiet=T)
        alldf <- alldf[alldf$m==0,] #remove measurements not on the hour
        alldf[,c("y","mo","d","h","m")] <- NULL
      
      # check that sensor IDs are present on all lines and warn if multiple sensor IDs found
        alldf$SensorID <- factor(alldf$SensorID)
        if(length(levels(alldf$SensorID)) >1) warning("Multiple SensorID values in file")
        if(length(levels(alldf$SensorID))==1) alldf$SensorID <- levels(alldf$SensorID) # to fill in NAs
        alldf$SensorID <- factor(alldf$SensorID)
      
      
      # calculate voltage
        alldf$volts <- alldf$power/1024 * 6.5
      
      # remove NA values for each sensor
        alldf$stErr <- ifelse(alldf$soiltemp==-127,T,F) # missing values for soil temp
        alldf$soiltemp[alldf$stErr==T] <- NA
      
        alldf$voltErr <- ifelse(alldf$volts<2,T,F) #not reading voltage
        alldf$volts[alldf$voltErr] <- NA
      
        alldf$vmcsErr <- ifelse(alldf$vmc_Surf_raw==65535,T,F) #missing surface vmc values
        alldf$vmc_Surf_raw[alldf$vmcsErr==T] <- NA
      
        alldf$vmcdErr <- ifelse(alldf$vmc_Deep_raw==65535,T,F) #missing deep vmc values
        alldf$vmc_Deep_raw[alldf$vmcdErr==T] <- NA
        
      # remove vmc values when soil is frozen
        alldf$vmc_Deep_raw[alldf$soiltemp<=0] <- NA
        alldf$vmc_Surf_raw[alldf$soiltemp<=0] <- NA
      
      # remove raw values for voltage
        alldf$power <- NULL
      
      return(alldf)
    }

  
  
#### import data and apply calibration ####
  # import & clean raw B data
    filelist <- list.files(path=data_path,full.names = T)
    processed <- vector("list",length(filelist))
    
    
    for(i in 1:length(filelist)){
      rawdat <- readLines(filelist[i],warn=F)
      processed[[i]] <- Bcleaner(rawdat)
    }
    
    clean_dat <- do.call(rbind,processed)
  
  # import and set function for calibration
  c_mod <- read.csv(paste(Metadata_path,"calib_coefs.csv",sep=""),row.names=1)
  
  
  calibFunc <- function(resist,soiltemp) {
    resist.sq <- resist^2
    resist.cu <- resist^3
    
    vmc <- c_mod["(Intercept)",] +
      + c_mod["resist",]*resist + c_mod["resist.sq",]*resist.sq + c_mod["resist.cu",]*resist.cu +
      + c_mod["soiltemp",] * soiltemp + c_mod["resist:soiltemp",]*soiltemp*resist
    
    return(vmc)
  }
  
  # apply calibration to data
    clean_dat$vmc_Deep <- calibFunc(clean_dat$vmc_Deep_raw,clean_dat$soiltemp)
    clean_dat$vmc_Surf <- calibFunc(clean_dat$vmc_Surf_raw,clean_dat$soiltemp)
      # note that this assumes deep and shallow soil temp are the same
    
    
  # remove lines with no valid sensor reading
    no_data <- is.na(clean_dat$vmc_Deep) & is.na(clean_dat$vmc_Surf) & is.na(clean_dat$soiltemp)
    clean_dat <- clean_dat[!no_data,]
    clean_dat$SensorID <- factor(clean_dat$SensorID)
    
#### save data as a .csv ####
    write.csv(clean_dat,paste0(Out_path,"sensordata.csv"),row.names=F)
    