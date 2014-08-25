import requests
import json
import datetime
import dateutil.parser
import pytz
import locale
import willie

## /msg chanserv flags #yourchannel yourbot +t
class DataKraken:
	def __init__(self, calendarId, key):
		self.__url = "https://www.googleapis.com/calendar/v3/calendars/%s/events?" % calendarId.replace("@", "%40")
		
		self.__params = {
			"singleEvents": "True",
			"key": key
		}
	
	
	def getResponse(self):
		now = datetime.datetime.utcnow()
		then = now + datetime.timedelta(60)
		
		self.__params["timeMin"] = now.isoformat("T") + "Z"
		self.__params["timeMax"] = then.isoformat("T") + "Z"
		
		return requests.get(self.__url, params=self.__params).json()





class UpcomingEvents():
	def __init__(self, dk, loc = None):
		self.__dk = dk
		
		if loc is not None:
			locale.setlocale(locale.LC_ALL, loc)
	
	
	def __somethingToDt(self, something, defaultTzName):
		## Text to date time
		if "dateTime" in something:
			text = something["dateTime"]
		else:
			text = something["date"]
		
		dt = dateutil.parser.parse(text)
		
		## Fix missing tz
		if dt.tzinfo is None:
			if "timeZone" in something:
				dataTzName = something["timeZone"]
			else:
				dataTzName = defaultTzName
			
			dt = pytz.timezone(dataTzName).localize(dt)
		
		## Bring tinto default tz
		defaultTz = pytz.timezone(defaultTzName)
		return defaultTz.normalize(dt.astimezone(defaultTz))
	
	
	def __composeEventTitle(self, event):
		dt = event["startDt"]
		
		text  = dt.strftime("%a. der ")
		text += "%i.%i.%i" % (dt.day, dt.month, dt.year)
		
		if dt.hour != 0 or dt.minute != 0:
			text += dt.strftime(" %H:%M Uhr")
		
		text += ": "+event["summary"]
		
		return text
	
	
	def getDescription(self):
		## Get data
		data = self.__dk.getResponse()
		
		if len(data["items"]) == 0:
			return ""
		
		## Add start/end dates
		for item in data["items"]:
			item["startDt"] = self.__somethingToDt(item["start"], data["timeZone"])
			item["endDt"]   = self.__somethingToDt(item["end"],   data["timeZone"])
		
		## Sort by start date
		data["items"].sort(key=lambda x: x["startDt"])
		
		return self.__composeEventTitle(data["items"][0])

section = "caltopic"
lastTopic = ""


def configure(config):
	if config.option('Show extra information about Bugzilla issues', False):
		config.add_section(section)
		config.add_list(
			section, "calendar_id",
			"The ID of your calendar (e.g. fCbqKKPE48nCfBTf3SdNUp5x42@group.calendar.google.com)",
			"Calendar ID:"
		)
		config.add_list(
			section, "api_key",
			"Your API key (e.g. H28LL_LfatPaLDXQWSqi-xPaQZD)",
			"API key:"
		)
		config.add_list(
			"core", "locale",
			"The optional locale for the whole willie process, if this module is loaded. If not provided, the locale won't be changed (e.g. de_DE.utf8)",
			"Locale:"
		)
		config.add_list(
			section, "mask",
			"The optional topic mask where \"%%s\" will be replaced by the upcoming event. If not provided \"%%s\" will be used (e.g. \"Wellcome to #channel | %%s\")",
			"Topic mask:"
		)


def setup(bot):
	global dk
	global ue
	
	if not bot.config.has_option(section, "calendar_id") or not bot.config.has_option(section, "api_key"):
		raise ConfigurationError("Not all ([%s] calendar_id and api_key) settings are defined" % section)
	
	dk = DataKraken(
		bot.config.caltopic.calendar_id,
		bot.config.caltopic.api_key
	)
	
	ue = UpcomingEvents(dk, bot.config.core.locale)


@willie.module.interval(60*30)
def f_time(bot):
	global lastTopic
	
	dsc = ue.getDescription()
	
	if bot.config.caltopic.mask is not None:
		topic = bot.config.caltopic.mask % dsc
	else:
		topic = dsc
	
	if topic == lastTopic:
		bot.debug("caltopic", "Topic has not changed", "verbose")
		return
	
	lastTopic = topic
	
	for channel in bot.config.caltopic.get_list("channels"):
		bot.msg("ChanServ", "topic %s %s" % (channel, topic))

