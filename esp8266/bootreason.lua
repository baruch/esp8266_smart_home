local reason, extreason, cause, epc1, epc2, epc3, vaddr, depc = node.bootreason()

local reason_str = {
    [1] = 'power on',
    [2] = 'reset',
    [3] = 'hardware reset via reset pin',
    [4] = 'watchdog reset',
}

local extreason_str = {
    [0] = 'power on',
    [1] = 'hardware watchdog reset',
    [2] = 'exception reset',
    [3] = 'software watchdog reset',
    [4] = 'software restart',
    [5] = 'wake from deep sleep',
    [6] = 'external reset',
}

local name

if extreason == nil then
    name = reason_str[reason]
else
    name = extreason_str[extreason]
end

if name == nil then
    name = 'Unknown'
end
print('Boot Reason: '..name)

if extreason == 3 then
    print('Cause:', cause)
    print('EPC1:', epc1)
    print('EPC2:', epc2)
    print('EPC3:', epc3)
    print('Virtual Addr:', vaddr)
    print('DEPC:', depc)
    print('')
end
