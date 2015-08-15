assert('parse object') do
  assert_equal({"foo"=>"bar"}, JSON::parse('{"foo": "bar"}'))
end
assert('parse null') do
  assert_equal({"foo"=>nil}, JSON::parse('{"foo": null}'))
end
assert('parse array') do
  assert_equal "foo", JSON::parse('[true, "foo"]')[1] 
end
assert('parse multi-byte') do
  assert_equal({"あいうえお"=>"かきくけこ"}, JSON::parse('{"あいうえお": "かきくけこ"}'))
end
assert('stringify boolean') do
  assert_equal "true", JSON::stringify(true)
end
assert('stringify symbol') do
  assert_equal "\"symbol\"", JSON::stringify(:symbol)
end
assert('strnigify object with numeric value') do
  assert_equal '{"foo":"bar"}', JSON::stringify({"foo"=>"bar"})
end
assert('strnigify object with string value') do
  assert_equal '{"foo":1}', JSON::stringify({"foo"=> 1})
end
assert('stringify object with float value') do
  assert_equal '{"foo":2.5}', JSON::stringify({"foo"=> 2.5})
end
assert('stringify object with nil value') do
  assert_equal '{"foo":null}', JSON::stringify({"foo"=> nil})
end
assert('stringify object with boolean key and float value') do
  assert_equal '{"true":5}', JSON::stringify({true=> 5.0})
end
assert('stringify object with object key and float value') do
  assert_equal '{"{\"foo\"=>\"bar\"}":1.5}', JSON::stringify({{"foo"=> "bar"}=> 1.5})
end
assert('stringify empty array') do
  assert_equal "[]",  JSON::stringify([])
end
assert('strnigify array with few elements') do
  assert_equal "[1,true,\"foo\"]", JSON::stringify([1,true,"foo"]) 
end
assert('stringify object with several keys') do
  assert_equal '{"bar":2,"foo":1}', JSON::stringify({"bar"=> 2, "foo"=>1})
end
assert('stringify multi-byte') do
  assert_equal '{"foo":"ふー","bar":"ばー"}', JSON::stringify({"foo"=>"ふー", "bar"=> "ばー"})
end
assert('stringify escaped') do
  assert_equal '["\\\\"]', JSON::stringify(['\\'])
end
assert('stringify escaped quote') do
  assert_equal '["\\\\\\\\\""]', JSON::stringify(['\\\"'])
  s = JSON::stringify(['\\\"'])
  assert_equal '[', s[0]
  assert_equal '"', s[1]
  assert_equal '\\', s[2]; assert_equal '\\', s[3]
  assert_equal '\\', s[4]; assert_equal '\\', s[5]
  assert_equal '\\', s[6]; assert_equal '"', s[7]
  assert_equal '"', s[8]
  assert_equal ']', s[9]
end
