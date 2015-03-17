assert('parse object') do
  JSON::parse('{"foo": "bar"}') == {"foo"=>"bar"}
end
assert('parse null') do
  JSON::parse('{"foo": null}') == {"foo"=>nil}
end
assert('parse array') do
  JSON::parse('[true, "foo"]')[1] == "foo"
end
assert('parse multi-byte') do
  assert_equal(JSON::parse('{"あいうえお": "かきくけこ"}'), {"あいうえお"=>"かきくけこ"})
end
assert('stringify boolean') do
  JSON::stringify(true) == "true"
end
assert('stringify symbol') do
  JSON::stringify(:symbol) == "\"symbol\""
end
assert('strnigify object with numeric value') do
  JSON::stringify({"foo"=>"bar"}) == '{"foo":"bar"}'
end
assert('strnigify object with string value') do
  JSON::stringify({"foo"=> 1}) == '{"foo":1}'
end
assert('stringify object with float value') do
  JSON::stringify({"foo"=> 2.5}) == '{"foo":2.5}'
end
assert('stringify object with nil value') do
  JSON::stringify({"foo"=> nil}) == '{"foo":null}'
end
assert('stringify object with boolean key and float value') do
  JSON::stringify({true=> 2.5}) == '{"true":2.5}'
end
assert('stringify object with object key and float value') do
  JSON::stringify({{"foo"=> "bar"}=> 1.5}) == '{"{\"foo\"=>\"bar\"}":1.5}'
end
assert('stringify empty array') do
  JSON::stringify([]) == "[]"
end
assert('strnigify array with few elements') do
  JSON::stringify([1,true,"foo"]) == "[1,true,\"foo\"]"
end
assert('stringify object with several keys') do
  JSON::stringify({"bar"=> 2, "foo"=>1}) == '{"bar":2,"foo":1}'
end
assert('stringify multi-byte') do
  JSON::stringify({"foo"=>"ふー", "bar"=> "ばー"}) == '{"foo":"ふー","bar":"ばー"}'
end
assert('stringify escaped') do
  JSON::stringify(['\\']) == '["\\\\"]'
end
assert('stringify escaped quote') do
  JSON::stringify(['\\\"']) == '["\\\\\\\\\""]'
  s = JSON::stringify(['\\\"'])
  assert_equal '[', s[0]
  assert_equal '"', s[1]
  assert_equal '\\', s[2]; assert_equal '\\', s[3]
  assert_equal '\\', s[4]; assert_equal '\\', s[5]
  assert_equal '\\', s[6]; assert_equal '"', s[7]
  assert_equal '"', s[8]
  assert_equal ']', s[9]
end
