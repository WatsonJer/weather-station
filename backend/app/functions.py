 #!/usr/bin/python3


#################################################################################################################################################
#                                                    CLASSES CONTAINING ALL THE APP FUNCTIONS                                                                                                    #
#################################################################################################################################################


class DB:

    def __init__(self,Config):

        from math import floor
        from os import getcwd
        from os.path import join
        from json import loads, dumps, dump
        from datetime import timedelta, datetime, timezone 
        from pymongo import MongoClient , errors, ReturnDocument
        from urllib import parse
        from urllib.request import  urlopen 
        from bson.objectid import ObjectId  
       
      
        self.Config                         = Config
        self.getcwd                         = getcwd
        self.join                           = join 
        self.floor                      	= floor 
        self.loads                      	= loads
        self.dumps                      	= dumps
        self.dump                       	= dump  
        self.datetime                       = datetime
        self.ObjectId                       = ObjectId 
        self.server			                = Config.DB_SERVER
        self.port			                = Config.DB_PORT
        self.username                   	= parse.quote_plus(Config.DB_USERNAME)
        self.password                   	= parse.quote_plus(Config.DB_PASSWORD)
        self.remoteMongo                	= MongoClient
        self.ReturnDocument                 = ReturnDocument
        self.PyMongoError               	= errors.PyMongoError
        self.BulkWriteError             	= errors.BulkWriteError  
        self.tls                            = False # MUST SET TO TRUE IN PRODUCTION


    def __del__(self):
            # Delete class instance to free resources
            pass
 


    ####################
    # LAB 4 FUNCTIONS  #
    ####################
    
    # 1. CREATE FUNCTION TO INSERT DATA IN TO THE WEATHER COLLECTION
    def update(self,data):
        try:
            remotedb 	= self.remoteMongo('mongodb://%s:%s@%s:%s' % (self.username, self.password,self.server,self.port), tls=self.tls)
            result      = remotedb.ELET2415.weather.insert_one(data)

        except Exception as e:
            msg = str(e)
            if "duplicate" not in msg:
                print("addUpdate error ",msg)
            return False
        else:                  
            return True
        
    def getRange(self, start, end):
        '''Return readings between two datetime objects'''
        try:
            remotedb = self.remoteMongo(
                'mongodb://%s:%s@%s:%s' % (self.username, self.password, self.server, self.port),
                tls=self.tls
            )
            result = list(remotedb.ELET2415.weather.find({"timestamp":{"$gte":int(start),"$lte":int(end)}}, {"_id":0}).sort("timestamp",1))
            
        except Exception as e:
            print("getRange error:", str(e))
            return []        # return empty list on error
        else:
            return result
    
    def getFieldMMARByTimestamp(self, field, start, end):
        '''Returns min, max, avg and range for ANY field between two timestamps.'''
        try:
            remotedb = self.remoteMongo(
                'mongodb://%s:%s@%s:%s' % (self.username, self.password, self.server, self.port),
                tls=self.tls
            )
            result = list(remotedb.ELET2415.weather.aggregate([
                {"$match": {"timestamp": {"$gte": int(start),"$lte": int(end)}}},
                {"$group": {"_id": None, "values": {"$push": "$$ROOT." + field}}},
                {"$project": {
                    "_id":   0,
                    "max":   {"$max": "$values"},
                    "min":   {"$min": "$values"},
                    "avg":   {"$avg": "$values"},
                    "range": {"$subtract": [{"$max": "$values"}, {"$min": "$values"}]}
                }}
            ]))
        except Exception as e:
            print(f"getFieldMMARByTimestamp error: {str(e)}")
            return None
        return result[0] if result else None
    
    def getFrequencyDistro(self, field, start, end):
        '''Returns frequency distribution for any sensor field between two timestamps.'''
        try:
            remotedb = self.remoteMongo(
                'mongodb://%s:%s@%s:%s' % (self.username, self.password, self.server, self.port),
                tls=self.tls
            )
            result = list(remotedb.ELET2415.weather.aggregate([
                {"$match": {"timestamp": {"$gte": int(start),
                                          "$lte": int(end)}}},
                {"$bucket": {
                    "groupBy":    "$" + field,
                    "boundaries": [0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100],
                    "default":    "outliers",
                    "output":     {"count": {"$sum": 1}}
                }}
            ]))
        except Exception as e:
            print(f"getFrequencyDistro error: {str(e)}")
            return None
        return result

    
    


   



def main():
    from config import Config
    from time import time, ctime, sleep
    from math import floor
    from datetime import datetime, timedelta
    one = DB(Config)
 
 
    start = time() 
    end = time()
    print(f"completed in: {end - start} seconds")
    
if __name__ == '__main__':
    main()


    