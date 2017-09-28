<?php
//run using "php -d enable_dl=On extension=./kolabcalendaring.so test.php [--verbose]"

include("kolabformat.php");
include("kolabcalendaring.php");

/////// Basic unit test facilities

$errors = 0;
$verbose = preg_match('/\s(--verbose|-v)\b/', join(' ', $_SERVER['argv']));

function assertequal($got, $expect, $name) {
	global $verbose, $errors;

	if ($got == $expect) {
		if ($verbose)
			print "OK - $name\n";
		return true;
	}
	else {
		$errors++;
		print "FAIL - $name\n";
		print "-- Expected " . var_export($expect, true) . ", got " . var_export($got, true) . " --\n";
		return false;
	}
}

function asserttrue($arg, $name) {
	return assertequal($arg, true, $name);
}

function assertfalse($arg, $name) {
	return assertequal($arg, false, $name);
}


/////// Test EventCal recurrence

$xml = <<<EOF
<icalendar xmlns="urn:ietf:params:xml:ns:icalendar-2.0">
  <vcalendar>
    <properties>
      <prodid>
        <text>Libkolab-0.4 Libkolabxml-0.9</text>
      </prodid>
      <version>
        <text>2.0</text>
      </version>
      <x-kolab-version>
        <text>3.0dev1</text>
      </x-kolab-version>
    </properties>
    <components>
      <vevent>
        <properties>
          <uid>
            <text>DDDEBE616DB7480A003725D1D7C4C2FE-8C02E7EEB49870A2</text>
          </uid>
          <created><date-time>2012-10-23T11:04:53Z</date-time></created>
          <dtstamp><date-time>2012-10-23T13:04:53Z</date-time></dtstamp>
          <sequence>
            <integer>0</integer>
          </sequence>
          <class>
            <text>PUBLIC</text>
          </class>
          <dtstart>
            <parameters>
              <tzid>
                <text>/kolab.org/Europe/Paris</text>
              </tzid>
            </parameters>
            <date-time>2012-10-23T14:00:00</date-time>
          </dtstart>
          <dtend>
            <parameters>
              <tzid>
                <text>/kolab.org/Europe/Paris</text>
              </tzid>
            </parameters>
            <date-time>2012-10-23T15:30:00</date-time>
          </dtend>
          <rrule>
            <recur>
              <freq>DAILY</freq>
              <count>4</count>
              <interval>2</interval>
            </recur>
          </rrule>
          <summary>
            <text>Recurring with libkolab</text>
          </summary>
        </properties>
      </vevent>
    </components>
  </vcalendar>
</icalendar>
EOF;

$rdates = <<<EOF
<icalendar xmlns="urn:ietf:params:xml:ns:icalendar-2.0">
  <vcalendar>
    <properties>
      <prodid>
        <text>Roundcube-libkolab-0.9 Libkolabxml-1.1</text>
      </prodid>
      <version>
        <text>2.0</text>
      </version>
      <x-kolab-version>
        <text>3.1.0</text>
      </x-kolab-version>
    </properties>
    <components>
      <vevent>
        <properties>
          <uid>
            <text>49961C572093EC3FC125799C004A200F-Lotus_Notes_Generated</text>
          </uid>
          <created>
            <date-time>2014-02-28T12:57:42Z</date-time>
          </created>
          <dtstamp>
            <date-time>2014-02-28T12:57:42Z</date-time>
          </dtstamp>
          <sequence>
            <integer>0</integer>
          </sequence>
          <class>
            <text>PUBLIC</text>
          </class>
          <dtstart>
            <parameters>
              <tzid>
                <text>/kolab.org/Europe/Amsterdam</text>
              </tzid>
            </parameters>
            <date-time>2012-03-30T04:00:00</date-time>
          </dtstart>
          <dtend>
            <parameters>
              <tzid>
                <text>/kolab.org/Europe/Amsterdam</text>
              </tzid>
            </parameters>
            <date-time>2012-03-30T20:00:00</date-time>
          </dtend>
          <transp>
            <text>TRANSPARENT</text>
          </transp>
          <rdate>
            <date>2012-03-30</date>
            <date>2013-03-30</date>
            <date>2014-03-30</date>
            <date>2015-03-30</date>
            <date>2016-03-30</date>
            <date>2017-03-30</date>
            <date>2018-03-30</date>
            <date>2019-03-30</date>
            <date>2020-03-30</date>
            <date>2021-03-30</date>
          </rdate>
          <summary>
            <text>Geburtstag Jane Doe (30.03.1969)</text>
          </summary>
        </properties>
      </vevent>
    </components>
  </vcalendar>
</icalendar>
EOF;

$e = kolabformat::readEvent($xml, false);
$ec = new EventCal($e);

$rstart = new cDateTime(2012,8,1, 0,0,0);
# asserttrue($ec->getNextOccurence($rstart) instanceof cDateTime, "EventCal::getNextOccurence() returning cDateTime instance");

$next = new cDateTime($ec->getNextOccurence($rstart));
assertequal(
	sprintf("%d-%d-%d %02d:%02d:%02d", $next->year(), $next->month(), $next->day(), $next->hour(), $next->minute(), $next->second()),
	"2012-10-23 14:00:00",
	"EventCal first recurrence"
);

$next = new cDateTime($ec->getNextOccurence($next));
assertequal(
	sprintf("%d-%d-%d %02d:%02d:%02d", $next->year(), $next->month(), $next->day(), $next->hour(), $next->minute(), $next->second()),
	"2012-10-25 14:00:00",
	"EventCal second recurrence"
);

$end = new cDateTime($ec->getOccurenceEndDate($next));
assertequal(
	sprintf("%d-%d-%d %02d:%02d:%02d", $end->year(), $end->month(), $end->day(), $end->hour(), $end->minute(), $end->second()),
	"2012-10-25 15:30:00",
	"EventCal::getOccurenceEndDate"
);

$last = new cDateTime($ec->getLastOccurrence());
assertequal(
	sprintf("%d-%d-%d %02d:%02d:%02d", $last->year(), $last->month(), $last->day(), $last->hour(), $last->minute(), $last->second()),
	"2012-10-29 14:00:00",
	"EventCal::getLastOccurence"
);

// test event with RDATE list

$e = kolabformat::readEvent($rdates, false);
$ec = new EventCal($e);

$rstart = new cDateTime(2012,3,1, 0,0,0);
$next = new cDateTime($ec->getNextOccurence($rstart));
assertequal(
	sprintf("%d-%02d-%02d %02d:%02d:%02d", $next->year(), $next->month(), $next->day(), $next->hour(), $next->minute(), $next->second()),
	"2012-03-30 04:00:00",
	"RDATE first recurrence"
);

$next = new cDateTime($ec->getNextOccurence($next));
assertequal(
	sprintf("%d-%02d-%02d %02d:%02d:%02d", $next->year(), $next->month(), $next->day(), $next->hour(), $next->minute(), $next->second()),
	"2013-03-30 04:00:00",
	"RDATE next recurrence"
);

$last = new cDateTime($ec->getLastOccurrence());
assertequal(
	sprintf("%d-%02d-%d %02d:%02d:%02d", $last->year(), $last->month(), $last->day(), $last->hour(), $last->minute(), $last->second()),
	"2021-03-30 04:00:00",
	"RDATE last occurence"
);


// terminate with error status
exit($errors);
