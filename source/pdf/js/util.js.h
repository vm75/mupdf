"Error.prototype.toString = function() {\n"
"if (this.stackTrace) return this.name + ': ' + this.message + this.stackTrace;\n"
"return this.name + ': ' + this.message;\n"
"};\n"
"\n"
"var MuPDF = {\n"
"monthPattern: /Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec/i,\n"
"monthName: [\n"
"'January',\n"
"'February',\n"
"'March',\n"
"'April',\n"
"'May',\n"
"'June',\n"
"'July',\n"
"'August',\n"
"'September',\n"
"'October',\n"
"'November',\n"
"'December'\n"
"],\n"
"shortMonthName: [\n"
"'jan',\n"
"'feb',\n"
"'mar',\n"
"'apr',\n"
"'may',\n"
"'jun',\n"
"'jul',\n"
"'aug',\n"
"'sep',\n"
"'oct',\n"
"'nov',\n"
"'dec'\n"
"],\n"
"dayName: [\n"
"'Sunday',\n"
"'Monday',\n"
"'Tuesday',\n"
"'Wednesday',\n"
"'Thursday',\n"
"'Friday',\n"
"'Saturday'\n"
"],\n"
"dateFormats: [\n"
"'m/d',\n"
"'m/d/yy',\n"
"'mm/dd/yy',\n"
"'mm/yy',\n"
"'d-mmm',\n"
"'d-mmm-yy',\n"
"'dd-mm-yy',\n"
"'yy-mm-dd',\n"
"'mmm-yy',\n"
"'mmmm-yy',\n"
"'mmm d, yyyy',\n"
"'mmmm d, yyyy',\n"
"'m/d/yy h:MM tt',\n"
"'m/d/yy HH:MM'\n"
"],\n"
"timeFormats: [\n"
"'HH:MM',\n"
"'h:MM tt',\n"
"'HH:MM:ss',\n"
"'h:MM:ss tt'\n"
"],\n"
"};\n"
"\n"
"\n"
"var display = {\n"
"visible: 0,\n"
"hidden: 1,\n"
"noPrint: 2,\n"
"noView: 3,\n"
"};\n"
"\n"
"var border = {\n"
"b: 'beveled',\n"
"d: 'dashed',\n"
"i: 'inset',\n"
"s: 'solid',\n"
"u: 'underline',\n"
"};\n"
"\n"
"var color = {\n"
"transparent: [ 'T' ],\n"
"black: [ 'G', 0 ],\n"
"white: [ 'G', 1 ],\n"
"gray: [ 'G', 0.5 ],\n"
"ltGray: [ 'G', 0.75 ],\n"
"dkGray: [ 'G', 0.25 ],\n"
"red: [ 'RGB', 1, 0, 0 ],\n"
"green: [ 'RGB', 0, 1, 0 ],\n"
"blue: [ 'RGB', 0, 0, 1 ],\n"
"cyan: [ 'CMYK', 1, 0, 0, 0 ],\n"
"magenta: [ 'CMYK', 0, 1, 0, 0 ],\n"
"yellow: [ 'CMYK', 0, 0, 1, 0 ],\n"
"};\n"
"\n"
"color.convert = function (c, colorspace) {\n"
"switch (colorspace) {\n"
"case 'G':\n"
"if (c[0] === 'RGB')\n"
"return [ 'G', c[1] * 0.3 + c[2] * 0.59 + c[3] * 0.11 ];\n"
"if (c[0] === 'CMYK')\n"
"return [ 'CMYK', 1 - Math.min(1, c[1] * 0.3 + c[2] * 0.59 + c[3] * 0.11 + c[4])];\n"
"break;\n"
"case 'RGB':\n"
"if (c[0] === 'G')\n"
"return [ 'RGB', c[1], c[1], c[1] ];\n"
"if (c[0] === 'CMYK')\n"
"return [ 'RGB',\n"
"1 - Math.min(1, c[1] + c[4]),\n"
"1 - Math.min(1, c[2] + c[4]),\n"
"1 - Math.min(1, c[3] + c[4]) ];\n"
"break;\n"
"case 'CMYK':\n"
"if (c[0] === 'G')\n"
"return [ 'CMYK', 0, 0, 0, 1 - c[1] ];\n"
"if (c[0] === 'RGB')\n"
"return [ 'CMYK', 1 - c[1], 1 - c[2], 1 - c[3], 0 ];\n"
"break;\n"
"}\n"
"return c;\n"
"}\n"
"\n"
"color.equal = function (a, b) {\n"
"var i, n\n"
"if (a[0] === 'G')\n"
"a = color.convert(a, b[0]);\n"
"else\n"
"b = color.convert(b, a[0]);\n"
"if (a[0] !== b[0])\n"
"return false;\n"
"switch (a[0]) {\n"
"case 'G': n = 1; break;\n"
"case 'RGB': n = 3; break;\n"
"case 'CMYK': n = 4; break;\n"
"default: n = 0; break;\n"
"}\n"
"for (i = 1; i <= n; ++i)\n"
"if (a[i] !== b[i])\n"
"return false;\n"
"return true;\n"
"}\n"
"\n"
"var font = {\n"
"Cour: 'Courier',\n"
"CourB: 'Courier-Bold',\n"
"CourBI: 'Courier-BoldOblique',\n"
"CourI: 'Courier-Oblique',\n"
"Helv: 'Helvetica',\n"
"HelvB: 'Helvetica-Bold',\n"
"HelvBI: 'Helvetica-BoldOblique',\n"
"HelvI: 'Helvetica-Oblique',\n"
"Symbol: 'Symbol',\n"
"Times: 'Times-Roman',\n"
"TimesB: 'Times-Bold',\n"
"TimesBI: 'Times-BoldItalic',\n"
"TimesI: 'Times-Italic',\n"
"ZapfD: 'ZapfDingbats',\n"
"};\n"
"\n"
"var highlight = {\n"
"i: 'invert',\n"
"n: 'none',\n"
"o: 'outline',\n"
"p: 'push',\n"
"};\n"
"\n"
"var position = {\n"
"textOnly: 0,\n"
"iconOnly: 1,\n"
"iconTextV: 2,\n"
"textIconV: 3,\n"
"iconTextH: 4,\n"
"textIconH: 5,\n"
"overlay: 6,\n"
"};\n"
"\n"
"var scaleHow = {\n"
"proportional: 0,\n"
"anamorphic: 1,\n"
"};\n"
"\n"
"var scaleWhen = {\n"
"always: 0,\n"
"never: 1,\n"
"tooBig: 2,\n"
"tooSmall: 3,\n"
"};\n"
"\n"
"var style = {\n"
"ch: 'check',\n"
"ci: 'circle',\n"
"cr: 'cross',\n"
"di: 'diamond',\n"
"sq: 'square',\n"
"st: 'star',\n"
"};\n"
"\n"
"var zoomtype = {\n"
"fitH: 'FitHeight',\n"
"fitP: 'FitPage',\n"
"fitV: 'FitVisibleWidth',\n"
"fitW: 'FitWidth',\n"
"none: 'NoVary',\n"
"pref: 'Preferred',\n"
"refW: 'ReflowWidth',\n"
"};\n"
"\n"
"var util = {};\n"
"\n"
"util.printd = function (fmt, d) {\n"
"function padZeros(num, places) {\n"
"var s = String(num)\n"
"while (s.length < places)\n"
"s = '0' + s;\n"
"return s;\n"
"}\n"
"if (!d) return null;\n"
"var res = '';\n"
"var tokens = fmt.match(/(m+|d+|y+|H+|h+|M+|s+|t+|[^mdyHhMst]+)/g);\n"
"var length = tokens ? tokens.length : 0;\n"
"var i;\n"
"for (i = 0; i < length; ++i) {\n"
"switch (tokens[i]) {\n"
"case 'mmmm': res += MuPDF.monthName[d.getMonth()]; break;\n"
"case 'mmm': res += MuPDF.monthName[d.getMonth()].substring(0, 3); break;\n"
"case 'mm': res += padZeros(d.getMonth()+1, 2); break;\n"
"case 'm': res += d.getMonth()+1; break;\n"
"case 'dddd': res += MuPDF.dayName[d.getDay()]; break;\n"
"case 'ddd': res += MuPDF.dayName[d.getDay()].substring(0, 3); break;\n"
"case 'dd': res += padZeros(d.getDate(), 2); break;\n"
"case 'd': res += d.getDate(); break;\n"
"case 'yyyy': res += d.getFullYear(); break;\n"
"case 'yy': res += d.getFullYear()%100; break;\n"
"case 'HH': res += padZeros(d.getHours(), 2); break;\n"
"case 'H': res += d.getHours(); break;\n"
"case 'hh': res += padZeros((d.getHours()+11)%12+1, 2); break;\n"
"case 'h': res += (d.getHours()+11)%12+1; break;\n"
"case 'MM': res += padZeros(d.getMinutes(), 2); break;\n"
"case 'M': res += d.getMinutes(); break;\n"
"case 'ss': res += padZeros(d.getSeconds(), 2); break;\n"
"case 's': res += d.getSeconds(); break;\n"
"case 'tt': res += d.getHours() < 12 ? 'am' : 'pm'; break;\n"
"case 't': res += d.getHours() < 12 ? 'a' : 'p'; break;\n"
"default: res += tokens[i];\n"
"}\n"
"}\n"
"return res;\n"
"}\n"
"\n"
"util.printx = function (fmt, val) {\n"
"function convertCase(str, cmd) {\n"
"switch (cmd) {\n"
"case '>': return str.toUpperCase();\n"
"case '<': return str.toLowerCase();\n"
"default: return str;\n"
"}\n"
"}\n"
"var cs = '=';\n"
"var res = '';\n"
"var i, m;\n"
"var length = fmt ? fmt.length : 0;\n"
"for (i = 0; i < length; ++i) {\n"
"switch (fmt.charAt(i)) {\n"
"case '\\\\':\n"
"if (++i < length)\n"
"res += fmt.charAt(i);\n"
"break;\n"
"case 'X':\n"
"m = val.match(/\\w/);\n"
"if (m) {\n"
"res += convertCase(m[0], cs);\n"
"val = val.replace(/^\\W*\\w/, '');\n"
"}\n"
"break;\n"
"case 'A':\n"
"m = val.match(/[A-Za-z]/);\n"
"if (m) {\n"
"res += convertCase(m[0], cs);\n"
"val = val.replace(/^[^A-Za-z]*[A-Za-z]/, '');\n"
"}\n"
"break;\n"
"case '9':\n"
"m = val.match(/\\d/);\n"
"if (m) {\n"
"res += m[0];\n"
"val = val.replace(/^\\D*\\d/, '');\n"
"}\n"
"break;\n"
"case '*':\n"
"res += val;\n"
"val = '';\n"
"break;\n"
"case '?':\n"
"if (val) {\n"
"res += convertCase(val.charAt(0), cs);\n"
"val = val.substring(1);\n"
"}\n"
"break;\n"
"case '=':\n"
"case '>':\n"
"case '<':\n"
"cs = fmt.charAt(i);\n"
"break;\n"
"default:\n"
"res += convertCase(fmt.charAt(i), cs);\n"
"break;\n"
"}\n"
"}\n"
"return res;\n"
"}\n"
"\n"
"util.printf = function () {\n"
"var i;\n"
"\n"
"if (arguments.length < 1)\n"
"return '';\n"
"\n"
"var res = '';\n"
"var arg_i = 1;\n"
"var regexp = /%[^dfsx]*[dfsx]|[^%]*/g;\n"
"var tokens = arguments[0].match(regexp);\n"
"var length = tokens ? tokens.length : 0;\n"
"\n"
"for (i = 0; i < length; i++) {\n"
"var tok = tokens[i];\n"
"if (tok.match(/^%/)) {\n"
"if (arg_i < arguments.length) {\n"
"var val = arguments[arg_i++];\n"
"var fval = '';\n"
"var neg = false;\n"
"var decsep_re = /^,[0123]/;\n"
"var flags_re = /^[+ 0#]+/;\n"
"var width_re = /^\\d+/;\n"
"var prec_re = /^\\.\\d+/;\n"
"var conv_re = /^[dfsx]/;\n"
"\n"
"tok = tok.replace(/^%/, '');\n"
"var decsep = tok.match(decsep_re);\n"
"if (decsep)\n"
"decsep = decsep[0];\n"
"tok = tok.replace(decsep_re, '');\n"
"var flags = tok.match(flags_re);\n"
"if (flags)\n"
"flags = flags[0];\n"
"tok = tok.replace(flags_re, '');\n"
"var width = tok.match(width_re);\n"
"if (width)\n"
"width = width[0];\n"
"tok = tok.replace(width_re, '');\n"
"var prec = tok.match(prec_re);\n"
"if (prec)\n"
"prec = prec[0];\n"
"tok = tok.replace(prec_re, '');\n"
"var conv = tok.match(conv_re);\n"
"if (conv)\n"
"conv = conv[0];\n"
"\n"
"prec = prec ? Number(prec.replace(/^\\./, '')) : 0;\n"
"var poschar = (flags && flags.match(/[+ ]/)) ? flags.match(/[+ ]/)[0] : '';\n"
"var pad = (flags && flags.match(/0/)) ? '0' : ' ';\n"
"\n"
"var point = '.';\n"
"var thou = '';\n"
"\n"
"if (decsep) {\n"
"switch (decsep) {\n"
"case ',0': thou = ','; break;\n"
"case ',1': break;\n"
"case ',2': thou = '.'; point = ','; break;\n"
"case ',3': point = ','; break;\n"
"}\n"
"}\n"
"\n"
"switch (conv) {\n"
"case 'x':\n"
"val = Math.floor(val);\n"
"neg = (val < 0);\n"
"if (neg)\n"
"val = -val;\n"
"\n"
"\n"
"while (val) {\n"
"fval = '0123456789ABCDEF'.charAt(val%16) + fval;\n"
"val = Math.floor(val/16);\n"
"}\n"
"\n"
"if (neg)\n"
"fval = '-' + fval;\n"
"else\n"
"fval = poschar + fval;\n"
"break;\n"
"\n"
"case 'd':\n"
"fval = String(Math.floor(val));\n"
"break;\n"
"\n"
"case 's':\n"
"\n"
"pad = ' ';\n"
"fval = String(val);\n"
"break;\n"
"\n"
"case 'f':\n"
"fval = String(val);\n"
"\n"
"if (prec) {\n"
"var frac = fval.match(/\\.\\d+/);\n"
"if (frac) {\n"
"frac = frac[0];\n"
"\n"
"\n"
"if (frac.length > prec+1)\n"
"frac = frac.substring(0, prec+1);\n"
"else if (frac.length < prec+1)\n"
"frac += new Array(prec+1-frac.length+1).join('0');\n"
"\n"
"fval = fval.replace(/\\.\\d+/, frac);\n"
"}\n"
"}\n"
"break;\n"
"}\n"
"\n"
"if (conv.match(/[fd]/)) {\n"
"if (fval >= 0)\n"
"fval = poschar + fval;\n"
"\n"
"if (point !== '.')\n"
"fval.replace(/\\./, point);\n"
"\n"
"if (thou) {\n"
"var intpart = fval.match(/\\d+/)[0];\n"
"intpart = new Array(2-(intpart.length+2)%3+1).join('0') + intpart;\n"
"intpart = intpart.match(/.../g).join(thou).replace(/^0*[,.]?/, '');\n"
"fval = fval.replace(/\\d+/, intpart);\n"
"}\n"
"}\n"
"\n"
"if (width && fval.length < width)\n"
"fval = new Array(width - fval.length + 1).join(pad) + fval;\n"
"\n"
"res += fval;\n"
"}\n"
"} else {\n"
"res += tok;\n"
"}\n"
"}\n"
"\n"
"return res;\n"
"}\n"
"\n"
"function AFMergeChange(event) {\n"
"return event.value;\n"
"}\n"
"\n"
"function AFMakeNumber(str) {\n"
"var nums = str.match(/\\d+/g);\n"
"if (!nums)\n"
"return null;\n"
"var res = nums.join('.');\n"
"if (str.match(/^[^0-9]*\\./))\n"
"res = '0.'+res;\n"
"return res * (str.match(/-/) ? -1.0 : 1.0);\n"
"}\n"
"\n"
"function AFExtractTime(dt) {\n"
"var ampm = dt.match(/(am|pm)/);\n"
"dt = dt.replace(/(am|pm)/, '');\n"
"var t = dt.match(/\\d{1,2}:\\d{1,2}:\\d{1,2}/);\n"
"dt = dt.replace(/\\d{1,2}:\\d{1,2}:\\d{1,2}/, '');\n"
"if (!t) {\n"
"t = dt.match(/\\d{1,2}:\\d{1,2}/);\n"
"dt = dt.replace(/\\d{1,2}:\\d{1,2}/, '');\n"
"}\n"
"return [dt, t?t[0]+(ampm?ampm[0]:''):''];\n"
"}\n"
"\n"
"function AFParseDateOrder(fmt) {\n"
"var i;\n"
"var order = '';\n"
"\n"
"\n"
"fmt += 'mdy';\n"
"\n"
"for (i = 0; i < fmt.length; i++) {\n"
"var c = fmt.charAt(i);\n"
"if ('ymd'.indexOf(c) !== -1 && order.indexOf(c) === -1)\n"
"order += c;\n"
"}\n"
"\n"
"return order;\n"
"}\n"
"\n"
"function AFMatchMonth(d) {\n"
"var m = d.match(MuPDF.monthPattern);\n"
"return m ? MuPDF.shortMonthName.indexOf(m[0].toLowerCase()) : null;\n"
"}\n"
"\n"
"function AFParseTime(str, d) {\n"
"if (!str)\n"
"return d;\n"
"\n"
"if (!d)\n"
"d = new Date();\n"
"\n"
"var ampm = str.match(/(am|pm)/);\n"
"var nums = str.match(/\\d+/g);\n"
"var hour, min, sec;\n"
"\n"
"if (!nums)\n"
"return null;\n"
"\n"
"sec = 0;\n"
"\n"
"switch (nums.length) {\n"
"case 3:\n"
"sec = parseInt(nums[2]);\n"
"case 2:\n"
"hour = parseInt(nums[0]);\n"
"min = parseInt(nums[1]);\n"
"break;\n"
"default:\n"
"return null;\n"
"}\n"
"\n"
"ampm = ampm && ampm[0]\n"
"\n"
"if (ampm === 'am' && hour < 12)\n"
"hour = 12 + hour;\n"
"if (ampm === 'pm' && hour >= 12)\n"
"hour = 0 + hour - 12;\n"
"\n"
"d.setHours(hour, min, sec);\n"
"\n"
"if (d.getHours() !== hour || d.getMinutes() !== min || d.getSeconds() !== sec)\n"
"return null;\n"
"\n"
"return d;\n"
"}\n"
"\n"
"function AFParseDateEx(d, fmt) {\n"
"var i;\n"
"var dt = AFExtractTime(d);\n"
"var nums = dt[0].match(/\\d+/g);\n"
"var order = AFParseDateOrder(fmt);\n"
"var text_month = AFMatchMonth(dt[0]);\n"
"var dout = new Date();\n"
"var year = dout.getFullYear();\n"
"var month = dout.getMonth();\n"
"var date = dout.getDate();\n"
"\n"
"dout.setHours(12, 0, 0);\n"
"\n"
"if (!nums)\n"
"return null;\n"
"\n"
"if (nums.length == 1 && nums[0].length == fmt.length && !text_month) {\n"
"\n"
"\n"
"var num = nums[0];\n"
"nums = [\"\"];\n"
"for (i = 0; i < fmt.length; i++)\n"
"{\n"
"nums[nums.length-1] += num.charAt(i);\n"
"if (i+1 < fmt.length && fmt.charAt(i) != fmt.charAt(i+1))\n"
"nums.push(\"\");\n"
"}\n"
"}\n"
"\n"
"\n"
"\n"
"\n"
"var total = nums.length + (text_month ? 1 : 0);\n"
"\n"
"if (total < 2 || nums.length > 3)\n"
"return null;\n"
"\n"
"if (nums.length < 3 && text_month) {\n"
"\n"
"month = text_month;\n"
"order = order.replace('m', '');\n"
"}\n"
"\n"
"order = order.substring(0, nums.length);\n"
"\n"
"\n"
"if (order === 'ym' || order === 'my' || (order === 'y' && text_month))\n"
"date = 1;\n"
"\n"
"for (i = 0; i < nums.length; i++) {\n"
"switch (order.charAt(i)) {\n"
"case 'y': year = parseInt(nums[i]); break;\n"
"case 'm': month = parseInt(nums[i]) - 1; break;\n"
"case 'd': date = parseInt(nums[i]); break;\n"
"}\n"
"}\n"
"\n"
"if (year < 100) {\n"
"if (fmt.search('yyyy') !== -1)\n"
"return null;\n"
"if (year >= 50)\n"
"year = 1900 + year;\n"
"else if (year >= 0)\n"
"year = 2000 + year;\n"
"}\n"
"\n"
"dout.setFullYear(year, month, date);\n"
"\n"
"if (dout.getFullYear() !== year || dout.getMonth() !== month || dout.getDate() !== date)\n"
"return null;\n"
"\n"
"return AFParseTime(dt[1], dout);\n"
"}\n"
"\n"
"function AFDate_KeystrokeEx(fmt) {\n"
"if (event.willCommit && !AFParseDateEx(event.value, fmt)) {\n"
"app.alert('The date/time entered ('+event.value+') does not match the format ('+fmt+') of the field [ '+event.target.name+' ]');\n"
"event.rc = false;\n"
"}\n"
"}\n"
"\n"
"function AFDate_Keystroke(index) {\n"
"AFDate_KeystrokeEx(MuPDF.dateFormats[index]);\n"
"}\n"
"\n"
"function AFDate_FormatEx(fmt) {\n"
"var d = AFParseDateEx(event.value, fmt);\n"
"event.value = d ? util.printd(fmt, d) : '';\n"
"}\n"
"\n"
"function AFDate_Format(index) {\n"
"AFDate_FormatEx(MuPDF.dateFormats[index]);\n"
"}\n"
"\n"
"function AFTime_Keystroke(index) {\n"
"if (event.willCommit && !AFParseTime(event.value, null)) {\n"
"app.alert('The value entered ('+event.value+') does not match the format of the field [ '+event.target.name+' ]');\n"
"event.rc = false;\n"
"}\n"
"}\n"
"\n"
"function AFTime_FormatEx(fmt) {\n"
"var d = AFParseTime(event.value, null);\n"
"event.value = d ? util.printd(fmt, d) : '';\n"
"}\n"
"\n"
"function AFTime_Format(index) {\n"
"AFTime_FormatEx(MuPDF.timeFormats[index]);\n"
"}\n"
"\n"
"function AFSpecial_KeystrokeEx(fmt) {\n"
"var cs = '=';\n"
"var val = event.value;\n"
"var res = '';\n"
"var i = 0;\n"
"var m;\n"
"var length = fmt ? fmt.length : 0;\n"
"\n"
"function convertCase(str, cmd) {\n"
"switch (cmd) {\n"
"case '>': return str.toUpperCase();\n"
"case '<': return str.toLowerCase();\n"
"default: return str;\n"
"}\n"
"}\n"
"\n"
"while (i < length) {\n"
"switch (fmt.charAt(i)) {\n"
"case '\\\\':\n"
"i++;\n"
"if (i >= length)\n"
"break;\n"
"res += fmt.charAt(i);\n"
"if (val && val.charAt(0) === fmt.charAt(i))\n"
"val = val.substring(1);\n"
"break;\n"
"\n"
"case 'X':\n"
"m = val.match(/^\\w/);\n"
"if (!m) {\n"
"event.rc = false;\n"
"break;\n"
"}\n"
"res += convertCase(m[0], cs);\n"
"val = val.substring(1);\n"
"break;\n"
"\n"
"case 'A':\n"
"m = val.match(/^[A-Za-z]/);\n"
"if (!m) {\n"
"event.rc = false;\n"
"break;\n"
"}\n"
"res += convertCase(m[0], cs);\n"
"val = val.substring(1);\n"
"break;\n"
"\n"
"case '9':\n"
"m = val.match(/^\\d/);\n"
"if (!m) {\n"
"event.rc = false;\n"
"break;\n"
"}\n"
"res += m[0];\n"
"val = val.substring(1);\n"
"break;\n"
"\n"
"case '*':\n"
"res += val;\n"
"val = '';\n"
"break;\n"
"\n"
"case '?':\n"
"if (!val) {\n"
"event.rc = false;\n"
"break;\n"
"}\n"
"res += convertCase(val.charAt(0), cs);\n"
"val = val.substring(1);\n"
"break;\n"
"\n"
"case '=':\n"
"case '>':\n"
"case '<':\n"
"cs = fmt.charAt(i);\n"
"break;\n"
"\n"
"default:\n"
"res += fmt.charAt(i);\n"
"if (val && val.charAt(0) === fmt.charAt(i))\n"
"val = val.substring(1);\n"
"break;\n"
"}\n"
"\n"
"i++;\n"
"}\n"
"\n"
"\n"
"if (val.length > 0)\n"
"event.rc = false;\n"
"\n"
"if (event.rc)\n"
"event.value = res;\n"
"else if (event.willCommit)\n"
"app.alert('The value entered ('+event.value+') does not match the format of the field [ '+event.target.name+' ] should be '+fmt);\n"
"}\n"
"\n"
"function AFSpecial_Keystroke(index) {\n"
"if (event.willCommit) {\n"
"switch (index) {\n"
"case 0:\n"
"if (!event.value.match(/^\\d{5}$/))\n"
"event.rc = false;\n"
"break;\n"
"case 1:\n"
"if (!event.value.match(/^\\d{5}[-. ]?\\d{4}$/))\n"
"event.rc = false;\n"
"break;\n"
"case 2:\n"
"if (!event.value.match(/^((\\(\\d{3}\\)|\\d{3})[-. ]?)?\\d{3}[-. ]?\\d{4}$/))\n"
"event.rc = false;\n"
"break;\n"
"case 3:\n"
"if (!event.value.match(/^\\d{3}[-. ]?\\d{2}[-. ]?\\d{4}$/))\n"
"event.rc = false;\n"
"break;\n"
"}\n"
"if (!event.rc)\n"
"app.alert('The value entered ('+event.value+') does not match the format of the field [ '+event.target.name+' ]');\n"
"}\n"
"}\n"
"\n"
"function AFSpecial_Format(index) {\n"
"var res;\n"
"switch (index) {\n"
"case 0:\n"
"res = util.printx('99999', event.value);\n"
"break;\n"
"case 1:\n"
"res = util.printx('99999-9999', event.value);\n"
"break;\n"
"case 2:\n"
"res = util.printx('9999999999', event.value);\n"
"res = util.printx(res.length >= 10 ? '(999) 999-9999' : '999-9999', event.value);\n"
"break;\n"
"case 3:\n"
"res = util.printx('999-99-9999', event.value);\n"
"break;\n"
"}\n"
"event.value = res ? res : '';\n"
"}\n"
"\n"
"function AFNumber_Keystroke(nDec, sepStyle, negStyle, currStyle, strCurrency, bCurrencyPrepend) {\n"
"if (sepStyle & 2) {\n"
"if (!event.value.match(/^[+-]?\\d*[,.]?\\d*$/))\n"
"event.rc = false;\n"
"} else {\n"
"if (!event.value.match(/^[+-]?\\d*\\.?\\d*$/))\n"
"event.rc = false;\n"
"}\n"
"if (event.willCommit) {\n"
"if (!event.value.match(/\\d/))\n"
"event.rc = false;\n"
"if (!event.rc)\n"
"app.alert('The value entered ('+event.value+') does not match the format of the field [ '+event.target.name+' ]');\n"
"}\n"
"}\n"
"\n"
"function AFNumber_Format(nDec, sepStyle, negStyle, currStyle, strCurrency, bCurrencyPrepend) {\n"
"var val = event.value;\n"
"var fracpart;\n"
"var intpart;\n"
"var point = sepStyle&2 ? ',' : '.';\n"
"var separator = sepStyle&2 ? '.' : ',';\n"
"\n"
"if (/^\\D*\\./.test(val))\n"
"val = '0'+val;\n"
"\n"
"var groups = val.match(/\\d+/g);\n"
"\n"
"if (!groups)\n"
"return;\n"
"\n"
"switch (groups.length) {\n"
"case 0:\n"
"return;\n"
"case 1:\n"
"fracpart = '';\n"
"intpart = groups[0];\n"
"break;\n"
"default:\n"
"fracpart = groups.pop();\n"
"intpart = groups.join('');\n"
"break;\n"
"}\n"
"\n"
"\n"
"intpart = intpart.replace(/^0*/, '');\n"
"if (!intpart)\n"
"intpart = '0';\n"
"\n"
"if ((sepStyle & 1) === 0) {\n"
"\n"
"\n"
"intpart = new Array(2-(intpart.length+2)%3+1).join('0') + intpart;\n"
"intpart = intpart.match(/.../g).join(separator).replace(/^0*/, '');\n"
"}\n"
"\n"
"if (!intpart)\n"
"intpart = '0';\n"
"\n"
"\n"
"fracpart += new Array(nDec+1).join('0');\n"
"fracpart = fracpart.substring(0, nDec);\n"
"\n"
"if (fracpart)\n"
"intpart += point+fracpart;\n"
"\n"
"if (bCurrencyPrepend)\n"
"intpart = strCurrency+intpart;\n"
"else\n"
"intpart += strCurrency;\n"
"\n"
"if (/-/.test(val)) {\n"
"switch (negStyle) {\n"
"case 0:\n"
"intpart = '-'+intpart;\n"
"break;\n"
"case 1:\n"
"break;\n"
"case 2:\n"
"case 3:\n"
"intpart = '('+intpart+')';\n"
"break;\n"
"}\n"
"}\n"
"\n"
"if (negStyle&1)\n"
"event.target.textColor = /-/.test(val) ? color.red : color.black;\n"
"\n"
"event.value = intpart;\n"
"}\n"
"\n"
"function AFPercent_Keystroke(nDec, sepStyle) {\n"
"AFNumber_Keystroke(nDec, sepStyle, 0, 0, '', true);\n"
"}\n"
"\n"
"function AFPercent_Format(nDec, sepStyle) {\n"
"var val = AFMakeNumber(event.value);\n"
"if (val == null) {\n"
"event.value = '';\n"
"return;\n"
"}\n"
"event.value = (val * 100) + '';\n"
"AFNumber_Format(nDec, sepStyle, 0, 0, '%', false);\n"
"}\n"
"\n"
"function AFSimple_Calculate(op, list) {\n"
"var i, res;\n"
"\n"
"switch (op) {\n"
"case 'SUM': res = 0; break;\n"
"case 'PRD': res = 1; break;\n"
"case 'AVG': res = 0; break;\n"
"}\n"
"\n"
"if (typeof list === 'string')\n"
"list = list.split(/ *, */);\n"
"\n"
"for (i = 0; i < list.length; i++) {\n"
"var field = this.getField(list[i]);\n"
"var value = Number(field.value);\n"
"switch (op) {\n"
"case 'SUM': res += value; break;\n"
"case 'PRD': res *= value; break;\n"
"case 'AVG': res += value; break;\n"
"case 'MIN': if (i === 0 || value < res) res = value; break;\n"
"case 'MAX': if (i === 0 || value > res) res = value; break;\n"
"}\n"
"}\n"
"\n"
"if (op === 'AVG')\n"
"res /= list.length;\n"
"\n"
"event.value = res;\n"
"}\n"
"\n"
"function AFRange_Validate(lowerCheck, lowerLimit, upperCheck, upperLimit) {\n"
"if (upperCheck && event.value > upperLimit)\n"
"event.rc = false;\n"
"if (lowerCheck && event.value < lowerLimit)\n"
"event.rc = false;\n"
"if (!event.rc) {\n"
"if (lowerCheck && upperCheck)\n"
"app.alert(util.printf('The entered value ('+event.value+') must be greater than or equal to %s and less than or equal to %s', lowerLimit, upperLimit));\n"
"else if (lowerCheck)\n"
"app.alert(util.printf('The entered value ('+event.value+') must be greater than or equal to %s', lowerLimit));\n"
"else\n"
"app.alert(util.printf('The entered value ('+event.value+') must be less than or equal to %s', upperLimit));\n"
"}\n"
"}\n"
"\n"
"/* Compatibility ECMAScript functions */\n"
"String.prototype.substr = function (start, length) {\n"
"if (start < 0)\n"
"start = this.length + start;\n"
"if (length === undefined)\n"
"return this.substring(start, this.length);\n"
"return this.substring(start, start + length);\n"
"}\n"
"Date.prototype.getYear = Date.prototype.getFullYear;\n"
"Date.prototype.setYear = Date.prototype.setFullYear;\n"
"Date.prototype.toGMTString = Date.prototype.toUTCString;\n"
"\n"
"app.plugIns = [];\n"
"app.viewerType = 'Reader';\n"
"app.language = 'ENU';\n"
"app.viewerVersion = NaN;\n"
