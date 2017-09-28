<?php
//run using:
// php -d enable_dl=On -dextension=/usr/local/lib/php/modules/kolabshared.so -dextension=/usr/local/lib/php/modules/kolabformat.so -dextension=/usr/local/lib/php/modules/kolabobject.so test.php

include("kolabformat.php");
include("kolabobject.php");

/////// Test Event
$e = new Event();
$e->setCreated(new cDateTime(2012,3,14, 9,5,30, true));
$e->setStart(new cDateTime(2012,7,31));
$e->setUid("uid");
$e->setPriority(1);

$xo = new XMLObject;
print $xo->writeEvent($e, kolabobject::KolabV2, "test.php");
print $xo->writeEvent($e, kolabobject::KolabV3, "test.php");

////// Test Contact
$c = new Contact();
$nc = new NameComponents;
$sn = new vectors;
$sn->push("Contact");
$nc->setSurnames($sn);
$gn = new vectors;
$gn->push("Sample");
$nc->setGiven($gn);
$c->setNameComponents($nc);
$c->setName("Sample Contact");
$em = new vectors;
$em->push("sample.v2@localhost");
$c->setEmailAddresses($em);

$xo = new XMLObject;
print $xo->writeContact($c, kolabobject::KolabV2, "test.php");
print "UID = " . $xo->getSerializedUID() . "\n\n";

print $xo->writeContact($c, kolabobject::KolabV3, "test.php");
print "UID = " . $xo->getSerializedUID() . "\n\n";


$dlxml = <<<EOL
<?xml version="1.0"?>
<distribution-list version="1.0">
  <uid>ebb1774429a2e03afafb31f233e23b42</uid>
  <body></body>
  <categories></categories>
  <creation-date>2010-11-25T18:02:32Z</creation-date>
  <last-modification-date>2011-07-23T09:06:38Z</last-modification-date>
  <sensitivity>public</sensitivity>
  <product-id>Horde::Kolab</product-id>
  <display-name>Another lista</display-name>
  <member>
    <display-name>Another  User</display-name>
    <smtp-address>other@debian-vm.local</smtp-address>
    <uid>a2cfdc52365ef429042413bf7717dc85</uid>
  </member>
  <member>
    <display-name>Sample A. User Jr.</display-name>
    <smtp-address>sample@debian-vm.local</smtp-address>
    <uid>f538c7e9ad5a63e4452b7db3bc291231</uid>
  </member>
</distribution-list>
EOL;

$xo = new XMLObject;
$dl = new DistList($xo->readDistList($dlxml, kolabobject::KolabV2));

echo $dl->uid() . "\n\n";
$ml = $dl->members();
for ($i=0; $i < $ml->size(); $i++) {
    $m = $ml->get($i);
    echo "Member [" . $m->type() . "]: " . $m->uid() . "; " . $m->email() . "\n";
}


$dl2 = new DistList();
$ml = new vectorcontactref;
$m1 = new ContactReference(ContactReference::UidReference, 'some-uid-value');
$ml->push($m1);
$m2 = new ContactReference(ContactReference::EmailReference, 'sample@localhost');
$ml->push($m2);

$dl2->setMembers($ml);
echo $xo->writeDistList($dl2, kolabobject::KolabV2);
echo $xo->writeDistList($dl2, kolabobject::KolabV3);

?>
