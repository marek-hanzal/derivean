export interface IHrefProps<TPath extends string = string> {
    href: TPath;
    query?: IHrefProps.Params.Infer<TPath>;
}

export namespace IHrefProps {
    export namespace Params {
        type IsParameter<TPart> = TPart extends `{${infer TParam}}` ? TParam : never;
        type Parse<TPath> = TPath extends `${infer A}/${infer B}` ? IsParameter<A> | Parse<B> : IsParameter<TPath>;
        export type Infer<TPath> = {
            [TKey in Parse<TPath>]: string | number;
        }
    }
}
