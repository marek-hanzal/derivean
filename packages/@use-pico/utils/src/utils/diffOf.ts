export const diffOf = <TType extends (string | number)[]>(alfa: TType, beta: TType): TType => alfa.filter((x) => !beta.includes(x)) as TType;
