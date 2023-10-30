import {type DateTime} from "@use-pico2/i18n";
import {StoreProvider} from "@use-pico2/store";
import {
    type FC,
    type PropsWithChildren
}                      from "react";
import {yearsOf}       from "../calendar/yearsOf";
import {YearsOfStore}  from "./YearsOfStore";

export namespace YearsOfProvider {
    export type Props = PropsWithChildren<{
        date?: DateTime;
        selected?: DateTime;
    }>;
}

export const YearsOfProvider: FC<YearsOfProvider.Props> = (
    {
        date,
        selected,
        ...props
    }) => {
    return <StoreProvider
        store={YearsOfStore}
        values={{
            years: yearsOf({
                date,
                selected,
            }),
        }}
        {...props}
    />;
};
