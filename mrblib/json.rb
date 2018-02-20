class Object
  def to_json(pretty=false)
    JSON::generate(self, pretty)
  end
end
