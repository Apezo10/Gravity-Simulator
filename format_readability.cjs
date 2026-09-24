const fs = require('fs');
const files = ['vectors.cpp', ...['src','tests'].flatMap(dir => fs.readdirSync(dir).filter(f => /\.(cpp|hpp)$/.test(f)).map(f => dir+'/'+f))];
function tokens(text) {
    const pattern = /\/\/[^\n]*|\/\*[\s\S]*?\*\/|"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'|[A-Za-z_]\w*|\d+(?:\.\d*)?(?:[eE][+-]?\d+)?[a-zA-Z]*|::|->|\+\+|--|<=|>=|==|!=|&&|\|\||\+=|-=|\*=|\/=|<<|>>|[^\s]/g;
    return [...text.matchAll(pattern)].filter(m => !m[0].startsWith('//') && !m[0].startsWith('/*'))
        .map(m => ({value:m[0], start:m.index, end:m.index+m[0].length}));
}
function expandControls(text) {
    const list = tokens(text), closes = new Map(), stack = [];
    const matching = {')':'(', ']':'[', '}':'{'};
    list.forEach((t,i) => {
        if (['(','[','{'].includes(t.value)) stack.push(i);
        else if (matching[t.value]) {
            const open = stack.pop();
            if (list[open]?.value !== matching[t.value]) throw Error('unbalanced delimiters');
            closes.set(open,i);
        }
    });
    function statementEnd(i) {
        if (list[i].value === '{') return closes.get(i)+1;
        if (['if','for','while','switch'].includes(list[i].value)) {
            const afterCondition = closes.get(i+1)+1;
            let end = statementEnd(afterCondition);
            if (list[i].value === 'if' && list[end]?.value === 'else') end = statementEnd(end+1);
            return end;
        }
        for (; i < list.length; ++i) {
            if (list[i].value === ';') return i+1;
            if (closes.has(i)) i = closes.get(i);
        }
        throw Error('No statement end');
    }
    const inserts = [];
    for (let i = 0; i < list.length; ++i) {
        let body;
        if (['if','for','while'].includes(list[i].value) && list[i+1]?.value === '(') {
            body = closes.get(i+1)+1;
            if (list[body]?.value === ';') continue;
        } else if (list[i].value === 'else' && list[i+1]?.value !== 'if') body = i+1;
        else continue;
        if (list[body].value === '{') continue;
        const end = statementEnd(body);
        inserts.push({position:list[body].start, text:'{\n'});
        inserts.push({position:list[end-1].end, text:'\n}'});
    }
    inserts.sort((a,b)=>b.position-a.position);
    for (const insert of inserts) text=text.slice(0,insert.position)+insert.text+text.slice(insert.position);
    return text;
}
function expandLines(text) {
    const list = tokens(text), edits = [], stack = [];
    for (let i = 0; i < list.length; ++i) {
        const t = list[i], next = list[i+1];
        if (['(','[','{'].includes(t.value)) stack.push(t.value);
        else if ([')',']','}'].includes(t.value)) stack.pop();
        if (!next || text.slice(t.end,next.start).includes('\n')) continue;
        if (t.value === ';' && stack.lastIndexOf('(') < stack.lastIndexOf('{')) edits.push(t.end);
        if (t.value === '{') {
            const lineEnd = text.indexOf('\n',t.end);
            const tail = text.slice(t.end,lineEnd < 0 ? text.length : lineEnd);
            if (tail.includes(';')) edits.push(t.end);
        }
        if (t.value === ':' && /\bcase\b|\bdefault\b/.test(text.slice(text.lastIndexOf('\n',t.start)+1,t.start))) edits.push(t.end);
    }
    for (const position of [...new Set(edits)].sort((a,b)=>b-a)) text=text.slice(0,position)+'\n'+text.slice(position);
    return text;
}
function layout(text) {
    const lines = text.split('\n'), output = [];
    let braces = 0, parens = 0;
    for (let line of lines) {
        line = line.trim();
        if (!line) { output.push(''); continue; }
        if (line.startsWith('#')) { output.push(line); continue; }
        const code = tokens(line);
        const closingBraces = (line.match(/^}+/)||[''])[0].length;
        const closingParen = line.startsWith(')') ? 1 : 0;
        const continuation = parens > 0 && !closingParen ? 1 : 0;
        let indent = Math.max(0, braces - closingBraces + continuation);
        if (/^(public|private|protected):/.test(line)) --indent;
        output.push('    '.repeat(Math.max(0,indent)) + line);
        for (const t of code) {
            if (t.value === '{') ++braces;
            if (t.value === '}') --braces;
            if (t.value === '(') ++parens;
            if (t.value === ')') --parens;
        }
    }
    return output.join('\n');
}
function spaceSections(text) {
    let lines = text.split('\n');
    // Move end-of-line comments above the statement they explain.
    lines = lines.flatMap(line => {
        const comment = line.indexOf('//');
        if (comment > 0 && line.slice(0,comment).trim() && !line.includes('"')) {
            const indent = line.match(/^\s*/)[0];
            return [indent+line.slice(comment),line.slice(0,comment).trimEnd()];
        }
        return [line];
    });
    let result = [];
    function blank(count) {
        while (result.length && !result[result.length-1].trim()) result.pop();
        if (result.length) for (let i=0;i<count;++i) result.push('');
    }
    for (let i=0;i<lines.length;++i) {
        const line = lines[i], trimmed=line.trim();
        if (trimmed.startsWith('//')) {
            let end=i;
            while (lines[end+1]?.trim().startsWith('//')) ++end;
            let next=end+1;
            while (lines[next] !== undefined && !lines[next].trim()) ++next;
            const control=/^(if|for|while|switch|do|try)\b/.test(lines[next]?.trim()||'');
            blank(control?2:1);
            result.push(...lines.slice(i,end+1));
            i=next-1;
            continue;
        }
        const previous = result[result.length-1]?.trim() || '';
        if (/^(if|for|while|switch|do|try)\b/.test(trimmed) && !previous.startsWith('//')) blank(2);
        else if (trimmed && previous === '}' && !/^(}|else|catch|while|\)|;|,)/.test(trimmed)) blank(1);
        if (trimmed.startsWith('using namespace') || /^(public|private|protected):/.test(trimmed)) blank(1);
        result.push(line);
    }
    return result.join('\n').replace(/\n{4,}/g,'\n\n\n').trim()+'\n';
}
for (const file of files) {
    let text=fs.readFileSync(file,'utf8').replace(/\r\n/g,'\n');
    text = expandControls(text);
    text = expandLines(text);
    text = layout(text);
    text = spaceSections(text);
    fs.writeFileSync(file,text);
}
