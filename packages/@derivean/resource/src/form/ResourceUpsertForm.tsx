import {ResourceIcon}       from "@derivean/ui";
import {TextInput}          from "@use-pico/form";
import {
    t,
    tx
}                           from "@use-pico/i18n";
import {
    type ComponentProps,
    type FC
}                           from "react";
import {ResourceTypeSelect} from "../input/ResourceTypeSelect";
import {ResourceUI}         from "../ui/ResourceUI";

export namespace ResourceUpsertForm {
    export type Props = Omit<ComponentProps<ResourceUI["MutationForm"]>, "inputs" | "defaultValues" | "Render">;
}

export const ResourceUpsertForm: FC<ResourceUpsertForm.Props> = (
    {
        entity,
        ...props
    }
) => {
    return <ResourceUI.MutationForm
        text={{
            submit:  entity ? t()`Update resource (label)` : t()`Create resource (label)`,
            success: {
                title:   t()`Success`,
                message: entity ? t()`Resource updated` : t()`Resource created`,
            }
        }}
        inputs={{
            name:   props => <TextInput
                label={t()`Resource name`}
                placeholder={tx()`Resource name (placeholder)`}
                {...props}
            />,
            typeId: props => <ResourceTypeSelect
                text={{
                    label:       t()`Resource type`,
                    placeholder: tx()`Resource type (placeholder)`,
                    selector:    {
                        title: t()`Resource type selector`,
                    }
                }}
                {...props}
            />,
        }}
        icon={<ResourceIcon/>}
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
