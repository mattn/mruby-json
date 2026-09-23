module JSON
  class JSONError < StandardError; end
  class ParserError < JSONError; end
  class NestingError < ParserError; end
  class GeneratorError < JSONError; end
end
