import {ItemIcon}       from "@derivean/ui";
import {TextInput}      from "@use-pico/form";
import {
    t,
    tx
}                       from "@use-pico/i18n";
import {
    type ComponentProps,
    type FC
}                       from "react";
import {ItemTypeSelect} from "../input/ItemTypeSelect";
import {ItemUI}         from "../ui/ItemUI";

export namespace ItemUpsertForm {
    export type Props = Omit<ComponentProps<ItemUI["MutationForm"]>, "inputs" | "defaultValues" | "Render">;
}

export const ItemUpsertForm: FC<ItemUpsertForm.Props> = (
    {
        entity,
        ...props
    }
) => {
    return <ItemUI.MutationForm
        text={{
            submit:  entity ? t()`Update item (label)` : t()`Create item (label)`,
            success: {
                title:   t()`Success`,
                message: entity ? t()`Item updated` : t()`Item created`,
            }
        }}
        inputs={{
            name:   props => <TextInput
                label={t()`Item name`}
                placeholder={tx()`Item name (placeholder)`}
                {...props}
            />,
            typeId: props => <ItemTypeSelect
                text={{
                    label:       t()`Item type`,
                    placeholder: tx()`Item type (placeholder)`,
                    selector:    {
                        title: t()`Item type selector`,
                    }
                }}
                {...props}
            />,
        }}
        icon={<ItemIcon/>}
        values={entity}
        toRequest={values => (entity ? {
            update: {
                update: values,
                query:  {
                    where: {
                        id: entity.id,
                    },
                },
            },
        } : {
            create: values,
        })}
        defaultValues={{
            name:   "",
            typeId: "",
        }}
        Render={({Input}) => <>
            <Input name={"name"}/>
            <Input name={"typeId"}/>
        </>}
        {...props}
    />;
};
