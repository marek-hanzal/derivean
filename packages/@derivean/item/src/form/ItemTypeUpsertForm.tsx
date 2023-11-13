import {ItemIcon}   from "@derivean/ui";
import {TextInput}  from "@use-pico/form";
import {
    t,
    tx
}                   from "@use-pico/i18n";
import {
    type ComponentProps,
    type FC
}                   from "react";
import {ItemTypeUI} from "../ui/ItemTypeUI";

export namespace ItemTypeUpsertForm {
    export type Props = Omit<ComponentProps<ItemTypeUI["MutationForm"]>, "inputs" | "defaultValues" | "Render">;
}

export const ItemTypeUpsertForm: FC<ItemTypeUpsertForm.Props> = (
    {
        entity,
        ...props
    }
) => {
    return <ItemTypeUI.MutationForm
        text={{
            submit:  entity ? t()`Update item type (label)` : t()`Create item type (label)`,
            success: {
                title:   t()`Success`,
                message: entity ? t()`Item type updated` : t()`Item type created`,
            }
        }}
        inputs={{
            name: props => <TextInput
                label={t()`Item type name`}
                placeholder={tx()`Item type name (placeholder)`}
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
            name: "",
        }}
        Render={({Input}) => <>
            <Input name={"name"}/>
        </>}
        {...props}
    />;
};
