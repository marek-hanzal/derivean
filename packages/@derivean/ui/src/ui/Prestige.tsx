import {
    css,
    Rating
}                from "@use-pico/client";
import {cn}      from "@use-pico/utils";
import {type FC} from "react";

const colors: Record<number, css.Color> = {
    0: "text-green",
    1: "text-blue",
    2: "text-yellow",
    3: "text-orange",
    4: "text-violet",
    5: "text-fuchsia",
} as const;

export namespace Prestige {
    export interface Props {
        prestige: number;
    }
}

export const Prestige: FC<Prestige.Props> = (
    {
        prestige,
    }
) => {
    return <Rating
        defaultValue={prestige}
        readOnly
        size={"xl"}
        emptySymbol={value => <div className={cn(
            colors[value] ?? "text-green",
            "i-tabler:star",
        )}/>}
        fullSymbol={value => <div className={cn(
            colors[value] ?? "text-green",
            "i-tabler:star-filled",
        )}/>}
        count={6}
    />;
};
