import {type IHrefProps} from "./IHrefProps";

export type INavigate<
    T,
    TPath extends string = string,
> = (href: IHrefProps<TPath> | TPath) => T;
