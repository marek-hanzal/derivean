import {cn} from "@use-pico/utils";
import {
    type FC,
    type ReactNode
}           from "react";

export namespace Preview {
    export interface Props {
        items: Item[];
    }

    export interface Item {
        key: string;
        label: ReactNode;
        value: ReactNode;
    }
}

export const Preview: FC<Preview.Props> = (
    {
        items,
    }
) => {
    return <div
        className={cn([
            "p-2",
            "border border-zinc-200 rounded-md",
            "shadow-sm shadow-zinc-200",
        ])}
    >
        {items.map(item => <div
            key={item.key}
            className={cn([
                "flex gap-4",
                "divide-x divide-zinc-100",
            ])}
        >
            <div
                className={cn([
                    "w-1/3",
                    "text-left font-semibold text-zinc-600",
                    "px-2",
                ])}
            >
                {item.label}
            </div>
            <div
                className={cn([
                    "w-2/3",
                    "px-2",
                ])}
            >
                {item.value}
            </div>
        </div>)}
    </div>;
};
