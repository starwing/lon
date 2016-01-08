local lon = require "lon"


local t = {
   1,2,3,
   ['foo'] = 'bar',
   4,5,6,
   {
      7,8,9,
      {
         a='a', b='b', c='c'
      }
   }
}

print(lon.encode(t))
print(lon.decode(lon.encode(t)))
print(lon.encode(lon.decode(lon.encode(t))))
