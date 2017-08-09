import urllib
import urllib2

url = 'https://raccoon.wjuni.com/ajax/report.php'
values = { 'test': 1 }
response = urllib2.urlopen(urllib2.Request(url, urllib.urlencode(values)))
exit(response.getcode()-200)