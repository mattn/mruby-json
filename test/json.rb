assert('parse object') do
  JSON.parse('{"foo": "bar"}') == {"foo"=>"bar"}
end
assert('parse array') do
  JSON.parse('[true, "foo"]')[1] == "foo"
end
assert('stringify boolean') do
  JSON.stringify(true) == "true"
end
assert('strnigify object with numeric value') do
  JSON.stringify({"foo"=>"bar"}) == '{"foo":"bar"}'
end
assert('strnigify object with string value') do
  JSON.stringify({"foo"=> 1}) == '{"foo":1}'
end
assert('stringify object with float value') do
  JSON.stringify({"foo"=> 2.3}) == '{"foo":2.3}'
end
assert('stringify object with boolean key and float value') do
  JSON.stringify({true=> 3.4}) == '{"true":3.4}'
end
assert('stringify object with object key and float value') do
  JSON.stringify({{"foo"=> "bar"}=> 1.2}) == '{"{\"foo\"=>\"bar\"}":1.2}'
end
assert('stringify empty array') do
  JSON.stringify([]) == "[]"
end
assert('strnigify array with few elements') do
  JSON.stringify([1,true,"foo"]) == "[1,true,\"foo\"]"
end
assert('stringify object with several keys') do
  JSON.stringify({"foo"=>1, "bar"=> 2}) == '{"foo":1,"bar":2}'
end
