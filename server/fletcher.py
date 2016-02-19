def fletcher16(content):
    sum1 = 0
    sum2 = 0

    for ch in content:
        val = ord(ch)
        sum1 += val
        sum1 %= 255
        sum2 += sum1
        sum2 %= 255

    return (sum1, sum2)    
